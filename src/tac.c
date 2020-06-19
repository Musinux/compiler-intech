#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h> /* used for log10() */
#include "symbol.h"
#include "buffer.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "utils.h"
#include "queue.h"
#include "tac.h"

/**
 * The Tree Address Code is an assembly-like language, with simpler primitives
 * Types of parameters:
 *  - local variable, with a user-defined name
 *  - argument, with a user-defined name
 *  - immediate value, known at compile-time (like 1, 2, etc.)
 *  - temporary variable, like tmp0, tmp1, etc. Likely to be stored in a CPU register
 *
 * The instruction set is:
 * <LABEL>:             # defines a label which can be referenced to jump to
 * JUMP <LABEL>         # unconditional jump
 * JUMP_LT <LABEL>      # jump if lower than
 * JUMP_LTE <LABEL>     # jump if lower than or equal
 * JUMP_GT <LABEL>      # jump if greater than or equal
 * JUMP_GTE <LABEL>     # jump if greater than or equal
 * JUMP_NEQ <LABEL>     # jump if not equal
 * JUMP_EQ <LABEL>      # jump if equal
 * CALL <FUNCTION>      # call a function
 * PARAM <VAR>          # load a function parameter
 * LOAD_ARG <RELATIVE_POSITION> <VAR_NAME>   # load a positional argument
 * DECL_LOCAL <RELATIVE_POSITION> <VAR_NAME> # declare a local variable
 * COMPARE <TMP/DIRECT> <TMP/VARIABLE>       # compare two values
 * ASSIGN <TMP/DIRECT> <VARIABLE>            # assign a value to a local or argument
 * <TMP> = <OP1> <OPERATOR> <OP2>            # execute a binary operation
 * <TMP> = <OP1>                             # assign a value to a tmp var
 */

extern symbol_t *global_table;
unsigned long label_number;
unsigned long tmp_number;
myqueue_t available_tmps = NULL;

/**
 * Calculates the string bytes necessary to represent an integer
 */
size_t integer_size (unsigned long integer_size)
{
  return integer_size == 0 ? 1 : log10(integer_size) + 1;
}

/**
 * prints a JUMP_* instruction to outfile
 */
void tac_instr_jump (FILE *outfile, ast_binary_e comp, char *iffalse)
{
  fprintf(outfile, "\tJUMP_%s %s\n", ast_cmp_to_string(comp), iffalse);
}

/**
 * prints a COMPARE instruction to outfile
 */
void tac_instr_cmp (FILE *outfile, char *op1, char *op2)
{
  fprintf(outfile, "\tCOMPARE %s %s\n", op1, op2);
}

/**
 * prints an ASSIGN instruction to outfile
 */
void tac_instr_assign (FILE *outfile, char *expr, ast_t *ast)
{
  fprintf(outfile, "\tASSIGN %s %s\n", \
      expr,
      ast->declaration.lvalue->var.name);
}

/**
 * prints a label to file
 */
void tac_instr_label (FILE *outfile, char *label)
{
  fprintf(outfile, "%s:\n", label);
}

/**
 * Checks whether a string is actually a tmp variable (= register variable)
 */
bool tac_is_tmp (char *tmp)
{
  return strlen(tmp) > 3 && tmp[0] == 't' && tmp[1] == 'm' && tmp[2] == 'p';
}

/**
 * Generates a temporary variable name
 * Reuses released tmps to avoid wasting registers
 * We only have a small amount of registers, so we need to reuse them
 * as much as possible
 */
char *tac_new_tmp ()
{
  if (!queue_isempty(available_tmps))
    return queue_dequeue(&available_tmps);

  size_t size = integer_size(tmp_number) + sizeof("tmp");
  char *tmp = malloc(sizeof(char) * size);
  snprintf(tmp, size, "tmp%lu", tmp_number);
  tmp_number++;
  return tmp;
}

/**
 * Releases a tmp variable, but first checks if it's actually a tmp variable
 * it could also be a local variable or an argument
 * instead of freeing the tmp var, it puts it into a queue to reuse it as soon
 * as possible
 */
void tac_release_tmp (char *tmp)
{
  assert(tmp != NULL);
  if (tac_is_tmp(tmp)) {
    queue_enqueue(&available_tmps, tmp);
  } else {
    free(tmp);
  }
}

/**
 * When the TAC code generation has been done, we free everything left
 */
void tac_free_tmps ()
{
  while (available_tmps)
    free(queue_dequeue(&available_tmps));
}

/**
 * Generates a new label name
 * labels are used for JUMP statements
 */
char *tac_new_label ()
{
  size_t size = integer_size(label_number) + sizeof("L");
  char *label = malloc(sizeof(char) * size);
  snprintf(label, size, "L%lu", label_number);
  label_number++;
  return label;
}

/**
 * generates the LOAD_ARG instructions
 * LOAD_ARG takes an immediate value as a parameter, specifying the 
 * relative address of the variable in the function's address space
 */
char * tac_gen_load_arg (char *name, size_t offset)
{
    size_t size = sizeof(char) * sizeof("\tLOAD_ARG  $") + \
                  integer_size(offset) + \
                  strlen(name) + 1;
    char *instr = malloc(size);
    assert(instr != NULL);
    snprintf(instr, size, "\tLOAD_ARG $%zu %s\n", offset, name);
    return instr;
}

/**
 * generates the DECL_LOCAL instructions
 * DECL_LOCAL takes an immediate value as a parameter, specifying the 
 * relative address of the variable in the function's address space
 */
char * tac_gen_load_local (char *name, size_t offset)
{
    size_t size = sizeof(char) * sizeof("\tDECL_LOCAL  $") + \
                  integer_size(offset) + \
                  strlen(name) + 1;
    char *instr = malloc(size);
    assert(instr != NULL);
    snprintf(instr, size, "\tDECL_LOCAL $%zu %s\n", offset, name);
    return instr;
}

/**
 * based on the list of parameters of a function and number of local variables
 * we generate the minimal instructions necessary to specify a function
 * we have:
 *  - ADD_STACK which specifies the number of bytes necessary
 *    to save all the local variables
 *  - LOAD_ARG to load the args from the registers to the stack
 *  - DECL_LOCAL to indicate where a local variable is stored
 *
 *  To calculate ADD_STACK we first need to go through all the locals and args
 *  of a function, hence why we store the LOAD_ARG and DECL_LOCAL in a queue
 *  before writing them to the file
 */
void tac_function_init (symbol_t *table, FILE *outfile)
{
  myqueue_t queue = NULL;
  size_t stack_size = 8; // stack starts at 8 because of saved stack pointer
  symbol_t *curr = table;

  while (curr) {
    ast_t *ast = curr->attributes;
    assert(ast->type == AST_VARIABLE);
    curr->rel_pos = stack_size;

    if (ast->var.type != AST_INTEGER) {
      printf("tac: Unknown variable type. exiting.\n");
      exit(1);
    }

    if (curr->type == SYM_PARAM)
      queue_enqueue(&queue, tac_gen_load_arg(curr->name, stack_size));
    else if (curr->type == SYM_VAR)
      queue_enqueue(&queue, tac_gen_load_local(curr->name, stack_size));
    else {
      printf("tac: Unexpected symbol. exiting.\n");
      exit(1);
    }
    stack_size += 8; // integer is 8 bytes (64 bits)
    curr = curr->next;
  }

  fprintf(outfile, "\tADD_STACK $%zu\n", stack_size);
  while (queue) {
    char *instr = queue_dequeue(&queue);
    fprintf(outfile, "%s", instr);
    free(instr);
  }
}

/**
 * A loop is a repeated sequence of instructions, with a condition to continue or stop
 * to go to a specific instruction, we create labels in the code we refer to
 * when we use JUMP instructions
 * our loops only work with typical while loops, (not do...while), so we need:
 *  - a label before the condition to jump to it after doing the loop instructions
 *  - a label after the instructions to jump to it if the loop has to be stopped
 *  - a label after the conditions but before the loop if we have nested
 *    conditions that need to refer to the start of the loop
 *      (a OR condition for example)
 */
void tac_loop (ast_t *ast, symbol_t *table, FILE *outfile)
{
  char *start = tac_new_label(),
       *iftrue = tac_new_label(),
       *iffalse = tac_new_label();

  tac_instr_label(outfile, start);
  tac_condition(ast->loop.condition, table, outfile, iftrue, iffalse, AST_BIN_AND);

  tac_instr_label(outfile, iftrue);
  tac_statement(ast->loop.stmt, table, outfile);
  fprintf(outfile, "\tJUMP %s\n", start);

  tac_instr_label(outfile, iffalse);
  free(start);
  free(iftrue);
  free(iffalse);
}

/**
 * Branching (if/else) is somewhat complicated.
 * You have to create multiple labels to make that work
 * - label_after refers to the instruction after all the branching is done
 *   if (cond) {
 *   } else {
 *   }
 *label_after: // refers to the section after the else
 * - iftrue refers to the section after the if_cond instructions 
 *    > because we move through the tree, the iftrue label shifts at each
 *    > iteration of the loop
 * - iffalse is either refering to the else statement or the label_after statement
 *    > if we do not have an 'invalid' branch, we jump to the end of the if tree
 * 
 * example:
 * 
 * L0:
 *   if (a < b) {
 * L1:
 *     a = 2;
 *   }
 *   else {
 * L2: 
 *     a = 3;
 *   }
 * L3:
 *   b = 4;
 */
void tac_branch (ast_t *ast, symbol_t *table, FILE *outfile)
{
  char *label_after = tac_new_label();
  ast_t *curr = ast;
  for (;;) {
    char *iftrue = tac_new_label();

    if (curr->type != AST_BRANCH) {
      tac_statement(curr, table, outfile);
      break;
    }

    char *iffalse = curr->branch.invalid ? tac_new_label() : label_after;

    tac_condition(curr->branch.condition, table, outfile, iftrue, iffalse, AST_BIN_AND);
    tac_instr_label(outfile, iftrue);
    free(iftrue);
    tac_statement(curr->branch.valid, table, outfile);

    if (!curr->branch.invalid)
      break;

    /* if we access this part, the if statement succeeded, so go to the end */
    fprintf(outfile, "\tJUMP %s\n", label_after);
    tac_instr_label(outfile, iffalse);
    free(iffalse);
    curr = curr->branch.invalid;
  }
  tac_instr_label(outfile, label_after);
  free(label_after);
}

/**
 * Generates a function call
 * - it first resolve the expressions in the arguments
 * - then it loads the params with  PARAM
 * - finally it calls the function
 *
 * example:
 * d = myfunction(a + b, c);
 * in this case:
 *  - we first calculate a + b
 *  - then c
 *  - then only we use PARAM instructions
 *
 * resulting operations:
 * tmp0 = a + b
 * tmp1 = c
 * PARAM tmp0
 * PARAM tmp1
 * CALL myfunction tmp2
 * ASSIGN tmp2 d
 */
char *tac_fncall (ast_t *ast, symbol_t *table, FILE *outfile)
{
  // on doit charger les arguments
  ast_list_t *curr = ast->call.args;
  myqueue_t params = NULL;
  while (curr) {
    ast_t *arg = curr->elem;
    queue_enqueue(&params, tac_expression(arg, table, outfile));
    curr = curr->next;
  }

  while (params) {
    char *var = queue_dequeue(&params);
    fprintf(outfile, "\tPARAM %s\n", var);
    tac_release_tmp(var);
  }
  char *tmp = tac_new_tmp();
  fprintf(outfile, "\tCALL %s %s\n", ast->call.name, tmp);
  return tmp;
}

/**
 * It only copies the name of the variable to ensure there are no use-after-free
 * conditions
 */
char *tac_variable (ast_t *ast, symbol_t *table, FILE *outfile)
{
  return copy_name(ast->var.name);
}

/**
 * Generates a string representation of an integer
 * Integer values always start with the symbol $ to be easily recognizable
 */
char *tac_integer (ast_t *ast, symbol_t *table, FILE *outfile)
{
  size_t size = integer_size(ast->integer) + 2;
  char *val = malloc(sizeof(char) * size);
  snprintf(val, size, "$%ld", ast->integer);
  return val;
}

/**
 * Transforms a binary operator into an assignment to a tmp variable
 */
char *tac_binary (ast_t *ast, symbol_t *table, FILE *outfile)
{
  char *left = tac_expression(ast->binary.left, table, outfile),
       *right = tac_expression(ast->binary.right, table, outfile),
       *op = ast_binary_to_string(ast->binary.op),
       *var = tac_new_tmp();
  fprintf(outfile, "\t%s = %s %s %s\n", var, left, op, right);
  tac_release_tmp(left);
  tac_release_tmp(right);
  return var;
}

/**
 * simple check to know if a string represents an "immediate value" (= a number)
 */
bool is_immediate (char *item)
{
  return item[0] == '$';
}

/**
 * The comparison instruction is constructed based on the limits of the x86 limitations
 *
 * the comparision looks like:
 * COMPARE $1 a
 * or
 * COMPARE tmp0 a
 * or
 * COMPARE tmp0 tmp1
 *
 * but you cannot compare directly two local/arg variables like
 * COMPARE a b // wrong
 * also, the immediate or register value MUST be the first argument
 * COMPARE a tmp0 // wrong
 *
 * the comparison operator sets special flags in the CPU so that the JUMP instructions
 * can do the right move based on the result of the previous comparison
 *
 */
ast_binary_e tac_comparison (ast_t *ast, symbol_t *table, FILE *outfile)
{
  /* we jump in the inverse case of the comparison
     so we get the opposite operator */
  ast_binary_e op = ast->binary.op;
  /* comparison is between two integers */
  char *operand1 = tac_expression(ast->binary.left, table, outfile);
  char *operand2 = tac_expression(ast->binary.right, table, outfile);

  /* the op1 must be a immediate value or a register */
  if (is_immediate(operand1) || !sym_search(table, operand1)) {
    /* if the op2 is an immediate value, we first store it into a tmp variable */
    if (is_immediate(operand2)) {
      char *tmp = tac_new_tmp();
      fprintf(outfile, "\t%s = %s\n", tmp, operand2);
      tac_instr_cmp(outfile, operand1, tmp);
      tac_release_tmp(tmp);
    }
    else
      tac_instr_cmp(outfile, operand1, operand2);
  }
  /* if the op1 is a local variable and op2 is an immediate value or a tmp var
   * we reverse the order of the operands in the cmp instruction */
  else if (is_immediate(operand2) || !sym_search(table, operand2)) {
    tac_instr_cmp(outfile, operand2, operand1);

    /* because we inversed the order, we also need to reverse the operator
     * Jump if greater than a b is transformed in Jump if lower or equal b a
     * != and == are equivalent because if b == a, then a == b */
    if (op != AST_BIN_DIFF && op != AST_BIN_EQ)
      op = ast_inv_cmp(op);
  }
  else {
    /* if none of the operands is a immediate/tmp var, we create a tmp var to store the value */
    char *tmp = tac_new_tmp();
    fprintf(outfile, "\t%s = %s\n", tmp, operand1);
    tac_instr_cmp(outfile, tmp, operand2);
    tac_release_tmp(tmp);
  }
  tac_release_tmp(operand1);
  tac_release_tmp(operand2);
  return op;
}

/**
 * A condition is evaluated that way:
 *   * It is expected that the top-level expression is a boolean expression
 *   * If we encounter an OR operator, we add a JUMP_IF_TRUE to the end of the expression
 *   * If we encounter an AND operator, we add a JUMP_IF_FALSE to the end of the expressio
 * (1 + 3) > (4 * 4) || 5 == 2
 *   || - >  - + - 1
 *               - 3
 *           - * - 4
 *               - 4
 *      - == - 5
 *           - 2
 *  Gives:
 *  tmp0 = 1 + 3
 *  tmp1 = 4 * 4
 *  COMPARE tmp0 tmp1
 *  JUMP_IF_GREATER LABEL_TRUE
 *  COMPARE 5 2
 *  JUMP_IF_NOT_EQUAL LABEL_FALSE
 *LABEL_TRUE:
 *  // STATEMENT IF TRUE
 *LABEL_FALSE:
 *  // STATEMENT IF FALSE
 *
 * Example2: (2 > 3 && 3 == 3) || 5 == 2
 *
 * It's a postfix depth-first operation
 */
void tac_condition (ast_t *ast, symbol_t *table, FILE *outfile,
    char *iftrue, char *iffalse, ast_binary_e parent_cond)
{
  if (ast->type != AST_BINARY) {
    printf("tac_condition: Expected a binary operator. exiting.\n");
    exit(1);
  }

  /* if it's a comparison operator (<, >, etc), it's straightforward
   * we create a jump instruction and stop here */
  if (ast_is_cmp(ast->binary.op)) {
    ast_binary_e comp = tac_comparison(ast, table, outfile);

    /* if a iffalse label exists, then we put a jump instruction which is
     * the reversed condition (the 'else if' is the inverse of the 'if') */
    if (iffalse)
      tac_instr_jump(outfile, ast_inv_cmp(comp), iffalse);
    else if (iftrue)
      tac_instr_jump(outfile, comp, iftrue);
    return;
  }

  /* if it's a boolean operator (OR, AND), we need to implement
   * the lazy evaluation scheme
   *  - if it's an AND operator (a AND b), we have to evaluate both parts  to know
   *  if we execute the iftrue instructions, but we can go to the ifelse label
   *  if the 'a' condition is invalid
   *  - if it's an OR operator (a OR b), we can go to the iftrue label if a is valid,
   *  but we need to go to the b part if 'a' is invalid
   */
  if (ast_is_bool(ast->binary.op)) {
    char *between_label = tac_new_label();
    ast_t *left = ast->binary.left,
          *right = ast->binary.right;

    if (ast->binary.op == AST_BIN_AND) {
      /* if the left part of the AND operator is not a comparison operator,
       * then it's a boolean operator, so it may need to jump to the second part of
       * the AND operator before ending all its operations
       * (think ((a OR b) AND c) where we may not need to evaluate 'b' to evaluate 'c'  */
      if (ast_is_cmp(left->binary.op))
        tac_condition(left, table, outfile, NULL, iffalse, -1);
      else
        tac_condition(left, table, outfile, between_label, iffalse, AST_BIN_AND);
    }
    else {
      /* if the right part of the OR operator is not a comparison operator,
       * then it's a boolean operator, so it may need to jump to the iffalse label
       * before ending all its operations
       * (think ((a AND b) OR c) where we may not need to evaluate 'c' to evaluate
       * the whole condition */
      if (ast_is_cmp(left->binary.op))
        tac_condition(left, table, outfile, iftrue, NULL, -1);
      else
        tac_condition(left, table, outfile, iftrue, between_label, AST_BIN_OR);
    }

    tac_instr_label(outfile, between_label);

    if (ast_is_cmp(right->binary.op)) {
      if (parent_cond == AST_BIN_OR)
        tac_condition(right, table, outfile, iftrue, NULL, -1);
      else
        tac_condition(right, table, outfile, NULL, iffalse, -1);
    }
    else
      tac_condition(right, table, outfile, iftrue, iffalse, parent_cond);

    free(between_label);
    return;
  }

  // si c'est une simple expression, on quitte
  printf("tac_condition: Expected either a comparison operator "
      "or a boolean operator. exiting.\n");
  exit(1);
}

/**
 * an expression can be either a binary operation, a simple integer,
 * a function call or a variable
 * this function returns the name of the tmp var in which the expression
 * result is stored
 */
char *tac_expression (ast_t *ast, symbol_t *table, FILE *outfile)
{
  switch (ast->type) {
  case AST_BINARY: return tac_binary(ast, table, outfile);
  case AST_INTEGER: return tac_integer(ast, table, outfile);
  case AST_FNCALL: return tac_fncall(ast, table, outfile);
  case AST_VARIABLE: return tac_variable(ast, table, outfile);
  default:
    printf("tac: Expected an expression. exiting.\n");
    exit(1);
  }
  return NULL;
}

/**
 * a compound statement is simply a list of statements, so we parse
 * them one by one like a linked list
 */
void tac_compound_statement (ast_t *ast, symbol_t *table, FILE *outfile)
{
  ast_list_t *curr = ast->compound_stmt.stmts;
  while (curr) {
    tac_statement(curr->elem, table, outfile);
    curr = curr->next;
  }
}

/**
 * an assignment is an expression saved into a variable
 */
void tac_assignment (ast_t *ast, symbol_t *table, FILE *outfile)
{
  char *expr = tac_expression(ast->assignment.rvalue, table, outfile);
  tac_instr_assign(outfile, expr, ast);
  tac_release_tmp(expr);
}

/* no need to store anything for empty declarations
   we could initialize the value to 0 if we'd like */
void tac_declaration (ast_t *ast, symbol_t *table, FILE *outfile)
{
  if (ast->declaration.rvalue)
    tac_assignment(ast, table, outfile);
}
  
/**
 * a return statement simply ends a function, with an optional
 * return value
 */
void tac_return (ast_t *ast, symbol_t *table, FILE *outfile)
{
  if (ast->ret.expr) {
    char *expr = tac_expression(ast->ret.expr, table, outfile);
    fprintf(outfile, "\tRETURN %s\n", expr);
    tac_release_tmp(expr);
  } else {
    fprintf(outfile, "\tRETURN\n");
  }
}

/**
 * A statement can be a declaration, an assigment, a return, a branch, a loop,
 * or a list of statements (compound statements)
 */
void tac_statement (ast_t *ast, symbol_t *table, FILE *outfile)
{
  switch(ast->type){
  case AST_DECLARATION: return tac_declaration(ast, table, outfile);
  case AST_ASSIGNMENT: return tac_assignment(ast, table, outfile);
  case AST_RETURN: return tac_return(ast, table, outfile);
  case AST_BRANCH: return tac_branch(ast, table, outfile);
  case AST_LOOP: return tac_loop(ast, table, outfile);
  case AST_COMPOUND_STATEMENT: return tac_compound_statement(ast, table, outfile);
  default:
    printf("tac_statement: Expected either a declaration, an assignment "
        "or a return statement. exiting.\n");
    exit(1);
  }
}

/**
 * Notre fonction doit être représentée par
 * - son nom
 * - la taille nécessaire en mémoire pour stocker toutes ses variables locales
 *    + ses arguments
 * - ses instructions
 * - son instruction de retour
 */
void tac_function (ast_t *ast, symbol_t *table, FILE *outfile)
{
  fprintf(outfile, "%s:\n", ast->function.name);
  tac_function_init(table, outfile);
  ast_list_t *curr = ast->function.stmts;
  while (curr) {
    tac_statement(curr->elem, table, outfile);
    curr = curr->next;
  }
}

/**
 * Generates the three address codes (tac)
 */
void tac_generator (ast_list_t *functions, FILE *outfile)
{
  label_number = 0;
  tmp_number = 0;
  available_tmps = NULL;

  while (functions) {
    ast_t *ast = functions->elem;
    symbol_t *table = sym_search(global_table, ast->function.name);
    assert(table != NULL);
    tac_function(ast, table->function_table, outfile);
    functions = functions->next;
  }

  tac_free_tmps();
}

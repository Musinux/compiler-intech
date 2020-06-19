/* vim: set tabstop=4:softtabstop=4:shiftwidth=4 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include "symbol.h"
#include "buffer.h"
#include "parser.h"
#include "ast.h"
#include "utils.h"
#include "stack.h"
#include "lexer.h"

extern symbol_t **pglobal_table;

void *parse_abort (buffer_t *buffer, const char *msg)
{
  printf("%s", msg);
  buf_print(buffer);
  exit(1);
  return NULL;
}

int parse_return_type (buffer_t *buffer)
{
  if (DEBUG) printf("parse_return_type\n");
  lexer_assert_twopoints(buffer, "parameters should be followed by ':'");

  char *lexem = lexer_getalphanum(buffer);
  if (lexem && !strcmp(lexem, "entier"))
    return AST_INTEGER;
  if (lexem && !strcmp(lexem, "rien"))
    return AST_VOID;
  else
    parse_abort(buffer,
        "Expected a valid type (either 'entier' or 'rien'). stopping. \n");
  return -1;
}

int parse_type (buffer_t *buffer)
{
  char *lexem = lexer_getalphanum(buffer);
  if (!lexem)
    parse_abort(buffer, "Expected a type. exiting.\n");

  int ret = -1;
  if (strcmp(lexem, "entier") == 0)
    ret = AST_INTEGER;
  else
    parse_abort(buffer, "Expected a valid type (either 'entier'). stopping. \n");

  free(lexem);
  return ret;
}


bool parse_is_type (buffer_t *buffer, char *str)
{
  return strcmp(str, "entier") == STREQUAL;
}

ast_list_t *parse_parameters (buffer_t *buffer, symbol_t **table)
{
  if (DEBUG) printf("parse_parameters\n");
  ast_list_t *params = NULL;
  ast_t *ast = NULL;
  
  lexer_assert_openbrace(buffer, "Expecting a '(' after function name");

  buf_lock(buffer);
  char next = buf_getchar_after_blank(buffer);
  if (next == ')') {
    buf_unlock(buffer);
    return params;
  }
  buf_rollback_and_unlock(buffer, 1);

  for (;;) {
    int type = parse_type(buffer);

    char *name = lexer_getalphanum(buffer);
    if (!name || isdigit(name[0]))
      parse_abort(buffer, "Expected an identifier. exiting.\n");

    if (sym_search(*table, name)) {
      printf("Identifier '%s' has already been declared. exiting.\n", name);
      buf_print(buffer);
      exit(1);
    }

    ast = ast_new_variable(name, type);
    ast_list_add(&params, ast);
    sym_add(table, sym_new(name, SYM_PARAM, ast));
    free(name);

    next = buf_getchar_after_blank(buffer);
    if (next == ')')
      break;

    if (next != ',')
      parse_abort(buffer, "Unexpected end of input. stopping. \n");
  }
  return params;
}

ast_t *parse_number (buffer_t *buffer)
{
  if (DEBUG) printf("parse_number\n");
  char *lexem = lexer_getnumber(buffer);
  char *invalid = NULL;
  long value = strtol(lexem, &invalid, 10);

  if (invalid && *invalid != '\0')
    parse_abort(buffer, "Number should only contain digits. exiting. \n");
  
  free(lexem);
  return ast_new_integer(value);
}

bool ast_check_types (ast_t *ast, ast_node_type_e type)
{
  symbol_t *sym;

  if (ast->type == AST_INTEGER && type == AST_INTEGER)
    return true;

  if (ast->type == AST_VARIABLE && ast->var.type == type)
    return true;

  if (ast->type == AST_BINARY) {
    if (type == AST_INTEGER && ast_is_arithmetic(ast->binary.op))
      return true;
    if (type == AST_BOOLEAN &&
        (ast_is_cmp(ast->binary.op) || ast_is_bool(ast->binary.op)))
      return true;
  }

  if (!(sym = sym_search(*pglobal_table, ast->call.name))) {
    printf("Unknown function name in function call. exiting.\n");
    exit(1);
  }

  if (ast->type == AST_FNCALL && sym->attributes->function.return_type == type)
    return true;

  return false;
}

ast_list_t *parse_arguments (buffer_t *buffer, symbol_t **table, symbol_t *function)
{
  if (DEBUG) printf("parse_arguments\n");
  ast_list_t *args = NULL;
  ast_t *ast = NULL;
  ast_list_t *param = function->attributes->function.params;
  for (;;) {
    ast = parse_expression(buffer, table);

    if (!ast_check_types(ast, param->elem->var.type))
      parse_abort(buffer, "Argument type does not match function definition. exiting.\n");

    ast_list_add(&args, ast);

    param = param->next;
    char next = buf_getchar(buffer);
    if (next == ')') {
      if (param) parse_abort(buffer, "Expected others arguments to function");
      return args;
    }

    if (next != ',')
      parse_abort(buffer, "Expected a ')' or a ',' after argument list");

    if (!param) {
      printf("Too many arguments to function '%s'. exiting.\n", function->name);
      buf_print(buffer);
      exit(1);
    }
  }
}

ast_t *parse_known_symbol (buffer_t *buffer, symbol_t **table)
{
  if (DEBUG) printf("parse_known_symbol\n");
  char *lexem = lexer_getalphanum(buffer);
  symbol_t *symbol = NULL;
  ast_t *ast = NULL;
  if (!lexem)
    parse_abort(buffer, "Expected an identifier. exiting.\n");

  if (!(symbol = sym_search(*table, lexem)) && \
      !(symbol = sym_search(*pglobal_table, lexem))) {
    printf("Identifier '%s' is used before declaration. exiting.\n", lexem);
    buf_print(buffer);
    exit(1);
  }

  if (SYM_ISVAR(symbol))
    ast = ast_new_variable(lexem, AST_GET_VARTYPE(symbol->attributes));
  else if (SYM_ISFUN(symbol)) {
    lexer_assert_openbrace(buffer,
        "function call should always be followed by (). exiting.\n");

    ast = ast_new_fncall(lexem, parse_arguments(buffer, table, symbol));
  }
  free(lexem);
  if (!ast)
    parse_abort(buffer, "Unknown symbol. exiting.\n");
  return ast;
}

ast_t * parse_stack_to_ast (mystack_t *ordered)
{
  if (stack_isempty(*ordered)) return NULL;
  ast_t *item = stack_pop(ordered);
  if (item->type == AST_UNARY && item->unary.op == AST_UN_PAREN) {
    ast_t *tmp = item->unary.operand;
    free(item);
    item = tmp;
  }

  if (DEBUG) ast_print_binary_or_integer(item);

  if (item->type == AST_BINARY) {
    if (!item->binary.right)
      item->binary.right = parse_stack_to_ast(ordered);
    if (!item->binary.left)
      item->binary.left = parse_stack_to_ast(ordered);
  }
  return item;
}

ast_t *parse_binary_expression (buffer_t *buffer, symbol_t **table)
{
  if (DEBUG) printf("parse_binary_expression\n");

  char *op = lexer_getop(buffer);

  assert(op != NULL);
  ast_binary_e type;
  // after a number, we should have an operator
  if (!strcmp(op, "+"))
    type = AST_BIN_PLUS;
  else if (!strcmp(op, "-"))
    type = AST_BIN_MINUS;
  else if (!strcmp(op, "*"))
    type = AST_BIN_MULT;
  else if (!strcmp(op, "/"))
    type = AST_BIN_DIV;
  else if (!strcmp(op, ">="))
    type = AST_BIN_GTE;
  else if (!strcmp(op, ">"))
    type = AST_BIN_GT;
  else if (!strcmp(op, "<="))
    type = AST_BIN_LTE;
  else if (!strcmp(op, "<"))
    type = AST_BIN_LT;
  else if (!strcmp(op, "!="))
    type = AST_BIN_DIFF;
  else if (!strcmp(op, "=="))
    type = AST_BIN_EQ;
  else if (!strcmp(op, "OU"))
    type = AST_BIN_OR;
  else if (!strcmp(op, "ET"))
    type = AST_BIN_AND;
  else
    parse_abort(buffer, "Expected a binary operator. exiting.\n");
  free(op);
  return ast_new_binary(type, NULL, NULL);
}

bool parse_expression_end (buffer_t *buffer)
{
  if (DEBUG) printf("parse_expression_end\n");
  char next = buf_getchar_rollback(buffer);
  return next == ';' || next == ')' || next == ',';
}

static
ast_t *parse_expression_ (buffer_t *buffer, symbol_t **table)
{
  if (DEBUG) printf("parse_expression_\n");
  ast_t *ast = NULL;

  buf_lock(buffer);
  char next = buf_getchar_after_blank(buffer);
  if (next == '(') {
    buf_unlock(buffer);

    ast = ast_new_unary(AST_UN_PAREN, parse_expression(buffer, table));
    lexer_assert_closebrace(buffer, "missing ')' at the end of the expression");
  }
  else if (isnumber(next)) {
    buf_rollback_and_unlock(buffer, 1);
    ast = parse_number(buffer);
  }
  else {
    buf_rollback_and_unlock(buffer, 1);
    ast = parse_known_symbol(buffer, table);
  }

  if (!ast)
    parse_abort(buffer, "Could not parse expression. exiting.\n");
  return ast;
}

/**
 * expressions can be composed of
 * - arithmetic operations
 * - function calls
 * - comparisons
 * example:
 *  myvar;
 *  1 + 2;
 *  myvar + 2;
 *  myfunction(1, 2, 3);
 *  myvar + 2 + myfunction();
 *  -1;
 *
 * Binary operators obey to precedence rules
 * rules:
 *  - div & mult always win
 *  - +/- second
 *  - comparison is third
 *    > cannot have two comparison operators on the same expression
 *    > unless they are split between ET / OU
 *  - conjonction operators always loose (ET / OU)
 *
 * example: a < b || b > 1 + 3 && c * 3 + 1 != -3
 * expected tree:
 * OU ─ < ─ a
 *        ➘ b
 *    ➘ ET ─ > ─ b
 *            ➘ + ─ 1
 *                ➘ 3
 *        ➘ != ─ + ─ * ─ c
 *                     ➘ 3
 *                 ➘ 1
 *             ➘ + ─ * ─ 3
 *                     ➘ 4
 *                 ➘ -3
 */
ast_t *parse_expression (buffer_t *buffer, symbol_t **table)
{
  if (DEBUG) printf("parse_expression\n");
  ast_t *curr = NULL,
        *last = NULL;
  mystack_t stack = NULL,
          ordered = NULL;
  bool isfinished = false;
  int next_expected = 0;

  do {
    if (stack_isempty(stack) ||
        ast_binary_priority(stack_top(stack)) <= ast_binary_priority(curr)) {
      if (curr != NULL && (stack_isempty(stack) || curr != stack_top(stack))) {
        stack_push(&stack, curr);
      }

      switch (next_expected)
      {
      case 0: curr = parse_expression_(buffer, table); break;
      case 1:
        if ((isfinished = parse_expression_end(buffer)))
          curr = NULL;
        break;
      case 2: curr = parse_binary_expression(buffer, table); break;
      }

      next_expected = (next_expected + 1) % 3;
    }
    else {
      do {
        last = stack_pop(&stack);
        stack_push(&ordered, last);
      }
      while (!stack_isempty(stack) &&
          ast_binary_priority(stack_top(stack)) >= ast_binary_priority(last));
    }
  }
  while (!isfinished || !stack_isempty(stack));

  ast_t *ast = parse_stack_to_ast(&ordered);
  if (DEBUG) printf("\n");
  return ast;
}

ast_t *parse_assignment (buffer_t *buffer, symbol_t **table, symbol_t *variable, char *lexem)
{
  if (DEBUG) printf("parse_assignment\n");
  ast_t *lvalue = NULL;

  if (!SYM_ISVAR(variable))
    parse_abort(buffer, "Assignment to something that is not a variable. exiting.\n");

  lvalue = ast_new_variable(lexem, AST_GET_VARTYPE(variable->attributes));

  lexer_assert_equalsign(buffer, "should have an equal sign");
  return ast_new_assignment(lvalue, parse_expression(buffer, table));
}

ast_t *parse_declaration (buffer_t *buffer, symbol_t **table)
{
  if (DEBUG) printf("parse_declaration\n");

  int type = parse_type(buffer);
  // add a SYM_VAR entry to the symbol table
  // evaluate the expression if it's an initialization;
  char *name = lexer_getalphanum(buffer);
  ast_t *lvalue = NULL;
  assert(name != NULL);

  if (isdigit(name[0]))
    parse_abort(buffer, "variable name cannot start with a number. exiting.");
 

  if (sym_search(*table, name)) {
    printf("Identifier '%s' has already been declared. exiting.\n", name);
    exit(1);
  }

  lvalue = ast_new_variable(name, type);
  sym_add(table, sym_new(name, SYM_VAR, lvalue));
  free(name);

  char next = buf_getchar_rollback(buffer);
  if (next == ';') {
    return ast_new_declaration(lvalue, NULL);
  }
  if (next == '=') {
    buf_forward(buffer, 1);
    return ast_new_declaration(lvalue, parse_expression(buffer, table));
  }
  parse_abort(buffer, "Expected either a '=' or a ';'\n");
  return NULL;
}

void parse_condition_start (
    buffer_t *buffer,
    symbol_t *fct,
    const char *expected,
    ast_t **condition,
    ast_t **valid)
{
  if (DEBUG) printf("parse_condition_start\n");

  char *lexem = lexer_getalphanum(buffer);

  if (strcmp(lexem, expected))
    parse_abort(buffer, "Condition should start with a si/tantque. exiting.\n");

  lexer_assert_openbrace(buffer, "condition should be followed by '('");
  *condition = parse_expression(buffer, &fct->function_table);
  
  if (!ast_check_types(*condition, AST_BOOLEAN))
    parse_abort(buffer, "Condition should contain a boolean expression. exiting.\n");

  lexer_assert_closebrace(buffer, "condition should be ended by a ')'");

  *valid = parse_statement(buffer, fct);
}

ast_t *parse_loop (buffer_t *buffer, symbol_t *fct)
{
  if (DEBUG) printf("parse_loop\n");
  ast_t *condition = NULL,
        *stmt = NULL;

  parse_condition_start(buffer, fct, "tantque", &condition, &stmt);

  return ast_new_loop(condition, stmt);
}

ast_t *parse_branch (buffer_t *buffer, symbol_t *fct)
{
  if (DEBUG) printf("parse_branch\n");
  ast_t *condition = NULL,
        *valid = NULL,
        *invalid = NULL;

  parse_condition_start(buffer, fct, "si", &condition, &valid);

  buf_lock(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (!lexem)
    buf_unlock(buffer);

  else {
    if (strcmp(lexem, "sinon") == STREQUAL) {
      buf_unlock(buffer);
      invalid = parse_statement(buffer, fct);
    } else {
      buf_rollback_and_unlock(buffer, strlen(lexem));
    }
  }

  return ast_new_branch(condition, valid, invalid);
}

ast_t *parse_compount_stmt (buffer_t *buffer, symbol_t *fct)
{
  if (DEBUG) printf("parse_compount_stmt\n");
  ast_list_t *stmts = NULL;

  while (buf_getchar_rollback(buffer) != '}')
    ast_list_add(&stmts, parse_statement(buffer, fct));

  buf_forward(buffer, 1); // skip '}'
  return ast_new_comp_stmt(stmts);
}

/* types of statements:
 *  - declaration / declaration + initialization OK
 *  - assignment OK
 *  - branching OK
 *  - any expression OK
*/
ast_t *parse_statement (buffer_t *buffer, symbol_t *fct)
{
  if (DEBUG) printf("parse_statement\n");
  symbol_t *symbol = NULL;
  ast_t *ast = NULL;
  // compound statement
  if (buf_getchar_rollback(buffer) == '{') {
    buf_forward(buffer, 1);
    return parse_compount_stmt(buffer, fct);
  }

  buf_lock(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (!lexem)
    parse_abort(buffer, "Did not find any suitable character. exiting.\n");

  if (DEBUG) printf("lexem: '%s'\n", lexem);
  /* branching */
  if (strcmp(lexem, "si") == STREQUAL) {
    buf_rollback_and_unlock(buffer, sizeof("si") - 1);
    free(lexem);
    return parse_branch(buffer, fct);
  }
  /* loop */
  if (strcmp(lexem, "tantque") == STREQUAL) {
    buf_rollback_and_unlock(buffer, sizeof("tantque") - 1);
    free(lexem);
    return parse_loop(buffer, fct);
  }
  /* return statement */
  if (strcmp(lexem, "retourner") == STREQUAL) {
    buf_unlock(buffer);
    ast_t *ret = parse_expression(buffer, &fct->function_table);

    if (!ast_check_types(ret, fct->attributes->function.return_type))
      parse_abort(buffer, "Wrong return expression.\n");

    ast = ast_new_return(ret);
  }
  /* declaration */
  else if (parse_is_type(buffer, lexem)) {
    buf_rollback_and_unlock(buffer, strlen(lexem));
    if (DEBUG) printf("found type '%s'\n", lexem);
    ast = parse_declaration(buffer, &fct->function_table);
  }
  /* assignment */
  else if ((symbol = sym_search(fct->function_table, lexem)) != NULL &&
      buf_getchar_rollback(buffer) == '=') {
    buf_unlock(buffer);
    if (DEBUG) printf("found symbol %s\n", symbol->name);
    ast = parse_assignment(buffer, &fct->function_table, symbol, lexem);
  }
  /* any expression */
  else {
    buf_rollback_and_unlock(buffer, strlen(lexem));
    if (DEBUG) printf("any expression\n");
    ast = parse_expression(buffer, &fct->function_table);
  }
  lexer_assert_semicolon(buffer, "Statement should end with a ';'");
  free(lexem);
  return ast;
}

ast_list_t *parse_function_body (buffer_t *buffer, symbol_t *fct)
{
  if (DEBUG) printf("parse_function_body\n");
  ast_list_t *stmts = NULL;

  lexer_assert_openbracket(buffer, "Function body should start with a '{'");

  while (buf_getchar_rollback(buffer) != '}') {
    ast_list_add(&stmts, parse_statement(buffer, fct));
  }

  lexer_assert_closebracket(buffer, "Function body should stop with a '}'");
  return stmts;
}

/*
 * fonction function_name (type arg1, type arg2) : return_type {
 *   instructions;
 * }
 */
ast_t *parse_function (buffer_t *buffer)
{
  if (DEBUG) printf("parse_function\n");
  int return_type;
  symbol_t *table = NULL;
  symbol_t *sym;
  ast_t *ast;
  ast_list_t *params;

  char *name = lexer_getalphanum(buffer);

  if (!name || isdigit(name[0]))
    parse_abort(buffer, "Identifier cannot start with a digit. stopping.\n");

  params = parse_parameters(buffer, &table);
  return_type = parse_return_type(buffer);

  ast = ast_new_function(name, return_type, params, NULL);
  sym = sym_new_function(name, SYM_FUNCTION, ast, table);
  sym_add(pglobal_table, sym);

  ast->function.stmts = parse_function_body(buffer, sym);

  printf("function %s: \n", name);
  sym_print_list(sym->function_table);

  free(name);
  return ast;
}

/**
 * This function generates ASTs for each global-scope function
 */
ast_list_t *parse (buffer_t *buffer)
{
  ast_list_t *functions = NULL;
  char *lexem = NULL;
  do  {
    lexem = lexer_getalphanum(buffer);
    if (lexem && strcmp(lexem, "fonction") == STREQUAL) {
      free(lexem);
      ast_list_add(&functions, parse_function(buffer));
    }
    else
      parse_abort(buffer, "Only functions are allowed on global scope.\n");
  } while (!buf_eof(buffer));

  if (!sym_search(*pglobal_table, "main")) {
    printf("The entrypoint 'main' function was not found. exiting.\n");
    exit(1);
  }

  if (DEBUG) printf("** end of file. **\n");
  return functions;
}

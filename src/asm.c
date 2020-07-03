#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include "utils.h"
#include "asm_sym.h"
#include "asm.h"
#include "buffer.h"
#include "lexer.h"

/**
 * The ASM module converts TAC representation into real Intel ASM x86_64
 * We are only using a subset of the x86_64 asm language.
 * The subset we use is the following.
 *
 * Argument parameters can be one of:
 *  - stack variable, with a memory offset relative to $rbp
 *  - immediate value, known at compile-time (like 1, 2, etc.) starting with a '$' symbol
 *  - CPU register
 *
 * Registers are special variables which are really quick for the CPU to read and write
 * Some have a specific role, some are generic
 * The most important registers are:
 *  * %rip: the Instruction Pointer, which points to the address of the next instruction to execute
 *     > We never set it ourselves, it's modified by the instrutions 'call' and 'ret'
 *  * %rbp: the Base Pointer, which is the address in the stack from which are calculated the current function's local variables
 *     > We have to manage it ourselves
 *  * %rsp: the Stack Pointer, which points to the top of the program stack
 *     > same
 *  * %rax: when a function returns a value, the value is stored in this register
 *  * %FLAGS: contains the current state of the processor, for our purpose mostly the result of the last comparison
 *     > zero bit: states whether the last operation resulted in 0
 *     > negative bit: states whether  the last operation was negative or positive
 *     > We don't have to manage this by ourselves
 * There are also General Purpose registers, which are useful as temporary variables
 *   %rax, %rbx, %r10, %r11, %r12, %r13, %r14, %r15
 * And then there are registers which are used to store the arguments of a function which is going to be called:
 *   %rdi, %rsi, %rdx, %rcx, %r8, %r9
 *
 * 
 * 
 * The instruction set we use is:
 * <LABEL>:                         # references a function
 * .L<LABEL>:                       # defines a label which can be referenced to jump to
 * push <REGISTER>                  # pushes the REGISTER value at the top of the stack
 * pop <REGISTER>                   # pops the top of the stack into the REGISTER
 * jmp <LABEL>                      # unconditional jump
 * jl <LABEL>                       # jump if lower than
 * jle <LABEL>                      # jump if lower than or equal
 * jg <LABEL>                       # jump if greater than or equal
 * jge <LABEL>                      # jump if greater than or equal
 * jne <LABEL>                      # jump if not equal
 * je <LABEL>                       # jump if equal
 * call <FUNCTION>                  # call a function
 * cmpq <REGISTER/IMMEDIATE>, <REGISTER>         # compare two values
 * movq $<IMMEDIATE>, -offset(%rbp)    # move an immediate value to a stack address
 *   > ex: movq $1, -16(%rbp) # we store 1 into the calculated address %rbp - 16
 * movq %<REGISTER>, -offset(%rbp)  # move a register to a stack address
 *   > ex: movq %rax, -16(%rbp)     # we store %rax content into
 *                                  #   the calculated address %rbp - 16
 * movq %<REGISTER>, %<REGISTER>    # move a register value in another register
 *   > ex: movq %rax, %rbx          # move %rax content into %rbx
 * movq -offset(%rbp), %<REGISTER>  # move a stack content into a register
 *   > ex: movq -16(%rbp), %rbx     # move content at address %rbp - 16 into %rbx
 * movq $<IMMEDIATE>, %<REGISTER>      # move a stack content into a register
 *   > ex: movq $1, %rbx            # move 1 into %rbx
 *
 * addq <DIRECT/REGISTER/RELATIVE>, <REGISTER> # add a value to the current value
 *                                          # of a register and store it into
 *                                          # the second operand
 *  > ex: addq %rax, %rbx           # add %rax to %rbx and store the result into %rbx
 * subq/divq/mulq # substract, divide, multiply
 * 
 */


#ifdef WIN32
char call_registers[6][5] = { "%rcx", "%rdx", "%r8", "%r9" };
#else
char call_registers[6][5] = { "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9" };
#endif
char general_purpose_registers[8][5] = {
  "%rax", "%rbx", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
};

/**
 * Checks whether a string represents a logical label
 */
bool is_internal_label (char *label)
{
  return label[0] == 'L' && label[1] >= '0' && label[1] <= '9';
}

/**
 * Checks whether a string represents a tmp variable
 * These variables will be stored in CPU registers
 */
bool is_tmp_var (char *tmp)
{
  return tmp[0] == 't' && tmp[1] == 'm' && tmp[2] == 'p' &&
    tmp[3] >= '0' && tmp[3] <= '9';
}

/**
 * Converts an immediate value representation to a 'long' integer
 * Direct values always start with a '$' symbol
 */
long asm_getimmediatevalue (buffer_t *buffer)
{
  char next = buf_getchar_after_blank(buffer);
  assert(next == '$');
  char *lexem = lexer_getnumber(buffer);

  char *invalid = NULL;
  long value = strtol(lexem, &invalid, 10);

  if (invalid && *invalid != '\0') {
    printf("Direct value should only contain digits. exiting. \n");
    exit(1);
  }

  free(lexem);
  return value;
}

/**
 * Parses a stack offset (relative position of a variable depending of the current
 * base pointer)
 * It verifies that the offset is neither negative or bigger than a INT value
 */
unsigned int asm_getoffset (buffer_t *buffer)
{
  long value = asm_getimmediatevalue(buffer);
  if (value < 0 || value > INT_MAX) {
    printf("Stack offset should not be negative nor > to INT_MAX. exiting\n");
    exit(1);
  }

  return (unsigned int)value;
}

/**
 * these functions are only convenience functions to generate the appropriate
 * instructions in assembly
 */
void asm_instr_register_to_var (char *op, char *reg, asm_symbol_t *symbol, FILE *outfile)
{ fprintf(outfile, "\t%s\t%s, -%u(%%rbp)\n", op, reg, symbol->pos); }

void asm_instr_immediate_to_var (char *op, long val, asm_symbol_t *symbol, FILE *outfile)
{ fprintf(outfile, "\t%s\t$%ld, -%u(%%rbp)\n", op, val, symbol->pos); }

void asm_instr_register_to_register (char *op, char *reg, char *reg2, FILE *outfile)
{ /* don't copy a register to itself */
  if (strcmp(reg, reg2)) fprintf(outfile, "\t%s\t%s, %s\n", op, reg, reg2);
}

void asm_instr_immediate_to_register (char *op, long val, char *reg, FILE *outfile)
{ fprintf(outfile, "\t%s\t$%ld, %s\n", op, val, reg); }

void asm_instr_var_to_register (char *op, asm_symbol_t *symbol, char *reg, FILE *outfile)
{ fprintf(outfile, "\t%s\t-%u(%%rbp), %s\n", op, symbol->pos, reg); }

/**
 * Parses a local variable based on its name
 */
asm_symbol_t *asm_getvar (buffer_t *buffer, asm_symbol_t *table)
{
  char *var = lexer_getalphanum(buffer);
  if (!var) {
    printf("asm_getvar: Expected a second operand. exiting.\n");
    exit(1);
  }
  asm_symbol_t *symbol = asm_sym_search(table, var);
  free(var);
  if (!symbol) {
    printf("asm_getvar: Assignment before declaration. exiting.\n");
    print_backtrace();
    exit(1);
  }
  return symbol;
}

/**
 * Gets a temporary variable and returns its associated register name
 * tmp0 => %rax
 * tmp1 => %rbx
 * see the general_purpose_registers array for the full list
 */
char *asm_get_tmp_reg (char *tmp)
{
  if (!tmp) {
    printf("Expected two operands. exiting.\n");
    exit(1);
  }
  if (!is_tmp_var(tmp)) {
    printf("Expected a temporary variable in the form tmp[0-9]+. exiting. (%s)\n", tmp);
    print_backtrace();
    exit(1);
  }
  char *invalid = NULL;
  long reg_nbr = strtol(&tmp[3], &invalid, 10);

  if (invalid && *invalid != '\0') {
    printf("tmp name should only contain digits after 'tmp' (%s). exiting. \n", tmp);
    exit(1);
  }
  if (reg_nbr >= MAX_GP_REGS) {
    printf("Exhaustion of General Purpose registers. exiting. \n");
    exit(1);
  }
  return general_purpose_registers[reg_nbr];
}

/**
 * Moves the %rsp stack pointer to add space into the stack
 * It's needed to keep the stack positions consistent between function calls
 * the stack goes downwards, that's why we substract `size` instead of adding it
 */
void asm_add_stack (buffer_t *buffer, FILE *outfile)
{
  if (DEBUG) printf("asm_add_stack\n");
  unsigned int size = asm_getoffset(buffer);
  asm_instr_immediate_to_register("subq", size, "%rsp", outfile);
}

/**
 * Add a local symbol to the symbol list (local variable)
 * The symbol is attached to an offset to %rsp
 */
void asm_decl_local (buffer_t *buffer, asm_symbol_t **table, FILE *outfile)
{
  if (DEBUG) printf("asm_decl_local\n");
  unsigned int pos = asm_getoffset(buffer);
  char *name = lexer_getalphanum(buffer);
  asm_symbol_t *symbol = asm_sym_new(pos, name);
  asm_sym_add(table, symbol);
}

/**
 * When we load a function argument, we add it to the symbol table, and we store
 * its value (which is first stored in a register) to the stack, with an attached offset
 * All the arguments passed to a function are passed in specific registers, see
 * the call_registers list to know in which order
 */
void asm_load_arg (buffer_t *buffer, asm_symbol_t **table, int *arg_count, FILE *outfile)
{
  unsigned int pos = asm_getoffset(buffer);
  char *name = lexer_getalphanum(buffer);
  asm_symbol_t *symbol = asm_sym_new(pos, name);
  asm_sym_add(table, symbol);
  if (*arg_count >= MAX_CALL_ARGS) {
    printf("Too many arguments for the current function. exiting.\n");
    exit(1);
  }
  asm_instr_register_to_var("movq", call_registers[*arg_count], symbol, outfile);
  (*arg_count)++;
}

/**
 * The return statement does two things:
 *  - if there is a return value, store it in the %rax register
 *  - then, call the 'leave' and 'ret' instructions which sets the %rsp and %rbp
 *    registers to their previous values
 *    leave does:
 *      - movq %rbp, %rsp # erase %rsp with the current %rbp (base pointer)
 *      - pop %rbp        # remove the last value from the stack and put it into %rbp
 *    ret does:
 *      - pop %rip        # remove the last value from the stack and put it into %rip
 */
void asm_return (buffer_t *buffer, asm_symbol_t *table, FILE *outfile)
{
  char next = buf_getchar_rollback(buffer);
  if (next == '$') {
    long val = asm_getimmediatevalue(buffer);
    asm_instr_immediate_to_register("movq", val, "%rax", outfile);
    fprintf(outfile, "\tleave\n"
                     "\tret\n");
    return;
  }

  char *lexem = lexer_getalphanum_rollback(buffer);
  if (!lexem) {
    printf("no lexem. exiting.\n");
    buf_print(buffer);
    exit(1);
  }

  if (asm_sym_search(table, lexem)) {
    asm_symbol_t *var = asm_getvar(buffer, table);
    asm_instr_var_to_register("movq", var, "%rax", outfile);
    fprintf(outfile, "\tleave\n"
                     "\tret\n");
    return;
  }

  char *tmp = lexer_getalphanum(buffer);
  char *reg = asm_get_tmp_reg(tmp);
  free(tmp);

  asm_instr_register_to_register("movq", reg, "%rax", outfile);
  fprintf(outfile, "\tleave\n"
                   "\tret\n");
}

/**
 * In Intel x86_64, binary operators always save the result into the 2nd operand,
 * except for the cmp operator.
 * The choices are:
 * immediate to var
 * register to var
 * immediate to register
 * register to register
 *
 * This function recognizes the patterns and sets the right syntax
 */
void asm_binary_op (buffer_t *buffer, asm_symbol_t *table, FILE *outfile, char *op)
{
  if (DEBUG) printf("asm_binary_op\n");
  bool in_immediatevalue = false;
  long val;
  char *reg = NULL;

  /* first, we check it the first operand is an immediate value or a register */
  if (buf_getchar_rollback(buffer) == '$') {
    val = asm_getimmediatevalue(buffer);
    in_immediatevalue = true;
  }
  else { // expect a temp variable
    char *tmp = lexer_getalphanum(buffer);
    reg = asm_get_tmp_reg(tmp);
    free(tmp);
  }

  /* we load the second operand into lexem */
  char *lexem = lexer_getalphanum_rollback(buffer);
  if (!lexem) {
    printf("no lexem. exiting.\n");
    buf_print(buffer);
    exit(1);
  }

  /* then we check if the second operand is a stack variable */
  if (asm_sym_search(table, lexem)) {
    asm_symbol_t *var = asm_getvar(buffer, table);
    if (in_immediatevalue)
      asm_instr_immediate_to_var(op, val, var, outfile);
    else
      asm_instr_register_to_var(op, reg, var, outfile);
  }
  /* if it was not a stack variable, it must be a register */
  else {
    char *lexem_out = lexer_getalphanum(buffer);
    char *reg_out = asm_get_tmp_reg(lexem_out);
    free(lexem_out);
    if (in_immediatevalue)
      asm_instr_immediate_to_register(op, val, reg_out, outfile);
    else
      asm_instr_register_to_register(op, reg, reg_out, outfile);
  }

  if (lexem) free(lexem);
}

/**
 * Generic function that sets the correct instruction form, expecting the second operand to be a register
 */
void asm_any_to_reg (buffer_t *buffer, asm_symbol_t *table, FILE *outfile,
    char *op, char *reg)
{
  if (DEBUG) printf("asm_any_to_reg\n");

  if (buf_getchar_rollback(buffer) == '$') {
    long val = asm_getimmediatevalue(buffer);
    asm_instr_immediate_to_register(op, val, reg, outfile);
    return;
  }

  char *lexem = lexer_getalphanum_rollback(buffer);
  if (asm_sym_search(table, lexem)) {
    asm_symbol_t *var = asm_getvar(buffer, table);
    asm_instr_var_to_register(op, var, reg, outfile);
  }
  else {
    char *lexem_in = lexer_getalphanum(buffer);
    char *reg_in = asm_get_tmp_reg(lexem_in);
    free(lexem_in);
    asm_instr_register_to_register(op, reg_in, reg, outfile);
  }
  if (lexem) free(lexem);
}

/**
 * Generic function that sets the correct instruction form, expecting the first operand to be a register
 */
void asm_reg_to_any (buffer_t *buffer, asm_symbol_t *table, FILE *outfile,
    char *op, char *reg)
{
  if (DEBUG) printf("asm_reg_to_any\n");
  char *lexem = lexer_getalphanum_rollback(buffer);
  if (asm_sym_search(table, lexem)) {
    asm_symbol_t *var = asm_getvar(buffer, table);
    asm_instr_register_to_var(op, reg, var, outfile);
  }
  else {
    char *lexem_out = lexer_getalphanum(buffer);
    char *reg_out = asm_get_tmp_reg(lexem_out);
    free(lexem_out);
    asm_instr_register_to_register(op, reg, reg_out, outfile);
  }
  if (lexem) free(lexem);
}

/**
 * Transforms a statement in the form tmpX = <op1> <operator> <op2> into two separate instructions
 * movq op1, tmpX
 * movq op2, tmpY
 * <operation> tmpY, tmpX
 */
void asm_arithmetic (buffer_t *buffer, asm_symbol_t *table, char *lexem, FILE *outfile)
{
  if (DEBUG) printf("asm_arithmetic\n");
  char *reg = asm_get_tmp_reg(lexem);
  lexer_assert_equalsign(buffer, "Expected a '=' after a 'tmp' variable");
  
  asm_any_to_reg(buffer, table, outfile, "movq", reg);

  buf_lock(buffer);
  char next = buf_getchar(buffer);
  if (next == '\n') {
    buf_rollback_and_unlock(buffer, 1);
    return;
  }

  buf_unlock(buffer);
  if (next != ' ') {
    printf("Expected a space or a '\\n' after an assignment. exiting.\n");
    exit(1);
  }

  next = buf_getchar(buffer);
  char *op = NULL;
  switch (next) {
  case '+': op = "addq"; break;
  case '-': op = "subq"; break;
  case '/': op = "divq"; break;
  case '*': op = "mulq"; break;
  default:
    printf("Unknown arithmetic operator. exiting.\n");
    exit(1);
  }
  asm_any_to_reg(buffer, table, outfile, op, reg);
}

/**
 * Transforms a JUMP instruction into its correct intel x86_64 form
 */
void asm_jump (buffer_t *buffer, char *lexem, FILE *outfile)
{
  char *op = NULL;
  if (!strcmp(lexem, "JUMP"))
    op = "jmp";
  else if (!strcmp(lexem, "JUMP_LT"))
    op = "jl";
  else if (!strcmp(lexem, "JUMP_LTE"))
    op = "jle";
  else if (!strcmp(lexem, "JUMP_GT"))
    op = "jg";
  else if (!strcmp(lexem, "JUMP_GTE"))
    op = "jge";
  else if (!strcmp(lexem, "JUMP_NEQ"))
    op = "jne";
  else if (!strcmp(lexem, "JUMP_EQ"))
    op = "je";
  else {
    printf("asm_jump: Unknown JUMP Operator. exiting.\n");
    exit(1);
  }

  char *label = lexer_getalphanum(buffer);
  fprintf(outfile, "\t%s\t.%s\n", op, label);
  free(label);
}

/**
 * Tranforms a PARAM instruction into a movq instruction to the correct parameter
 * based on the call_registers array
 */
void asm_param (buffer_t *buffer, asm_symbol_t *table, int *param_count, FILE *outfile)
{
  if (*param_count >= MAX_CALL_ARGS) {
    printf("asm_param: Too many parameters for a function. exiting.\n");
    exit(1);
  }

  asm_any_to_reg(buffer, table, outfile, "movq", call_registers[*param_count]);
  (*param_count)++;
}

/**
 * Tranforms a CALL instruction into the equivalent in x86_64, and also defines
 * the return value if applicable.
 * The return value of a function is always the %rax register
 */
void asm_call (buffer_t *buffer, asm_symbol_t *table, int *param_count, FILE *outfile)
{
  *param_count = 0;
  char *fnname = lexer_getalphanum(buffer);
  if (!fnname) {
    printf("Expected a function name after CALL. exiting.\n");
    exit(1);
  }
  fprintf(outfile, "\tcall\t%s\n", fnname);
  free(fnname);
  char next = buf_getchar_rollback(buffer);
  if (next == '\n') return;
  asm_reg_to_any(buffer, table, outfile, "movq", "%rax");
}

/**
 * There are two types of labels:
 *  - function labels, which are just the name of a function.
 *    > When we start a function, we need to set the function prolog,
 *      which is the definition of %rsp and %rbp registers.
 *      Both registers are needed to define the current function' address space
 *  - a simple numbered label, which is only useful for JUMP instructions
 */
void asm_label (buffer_t *buffer, FILE *outfile, int *arg_count, bool *is_main)
{
  if (DEBUG) printf("asm_label\n");
  char *label = lexer_getalphanum(buffer);
  assert(label != NULL);
  lexer_assert_twopoints(buffer, "expected a ':' after a label");
  lexer_assert_newline(buffer, "expected a '\\n' after a label");

  if (is_internal_label(label)) {
    fprintf(outfile, ".%s:\n", label);
  }
  else {
    if (strcmp(label, "main") == 0) {
      fprintf(outfile,
        "real_main:\n"
        "\tpushq\t%%rbp\n"
        "\tmovq\t%%rsp, %%rbp\n");
      *is_main = true;
    }
    else {
      *is_main = false;
      *arg_count = 0;
      /* 
      * This is the function prolog, storing the previous %rbp,
      * and setting the new %rbp to the previous %rsp
      * Which means: save the previous base address, and set the base address at the top of the stack, which is used to refer to local variables.
      */
      fprintf(outfile,
          "%s:\n"
          "\tpushq\t%%rbp\n"
          "\tmovq\t%%rsp, %%rbp\n",
          label);
    }
  }
  free(label);
}

void asm_program_arguments (buffer_t *buffer, FILE *outfile, int arg_count)
{
  fprintf(outfile, ".LC0:\n");
  fprintf(outfile, "\t.string \"%%d\\n\"\n");
  fprintf(outfile,
    "main:\n"
    "\tpushq\t%%rbp\n"
    "\tmovq\t%%rsp, %%rbp\n");
  
  /** nombre d'arguments de notre programme + variables argc et argv et %rbp **/
  fprintf(outfile, "\tsubq\t$%d, %%rsp\n", (arg_count + 2 + 1) * 8);

  char argv[] = "-16(%rbp)";
  // chargement de argc dans une variable locale
  fprintf(outfile, "\tmovq\t%s, -8(%%rbp)\n", call_registers[0]);
  // chargement de argv dans une variable locale
  fprintf(outfile, "\tmovq\t%s, %s\n", call_registers[1], argv);

  for (int i = 0; i < arg_count; i++) {
    // movq	-48(%rbp), %rax # on déplace argv dans %rax
    fprintf(outfile, "\tmovq %s, %%rax\n", argv);
    // déplacement dans le tableau argv, on commence à l'index 1 plutôt que 0
    // car l'index 0... c'est le nom du programme lui-même
    fprintf(outfile, "\taddq\t$%d, %%rax\n", 8 * (i + 1));
    // movq	(%rax), %rax    # et on charge le contenu à l'adresse de %rax argv[0]
    fprintf(outfile, "\tmovq\t(%%rax), %%rax\n");
    // movl	$10, %edx
    fprintf(outfile, "\tmovq\t$10, %s\n", call_registers[2]);
    // movl	$0, %esi
    fprintf(outfile, "\tmovq\t$0, %s\n", call_registers[1]);
    // movq	%rax, %rdi
    fprintf(outfile, "\tmovq\t%%rax, %s\n", call_registers[0]);
    // call	strtol@PLT # strtol(%rdi = argv[0], %rsi = NULL, %rdx = 10);
    fprintf(outfile, "\tcall	strtol@PLT\n");
    // movq	%rax, -8(%rbp)
    fprintf(outfile, "\tmovq\t%%rax, -%d(%%rbp)\n", (i + 3) * 8);
  }

  for (int i = 0; i < arg_count; i++) {
    fprintf(outfile, "\tmovq\t-%d(%%rbp), %s\n", (i + 3) * 8, call_registers[i]);
  }

  fprintf(outfile, "\tcall real_main\n");
  // représente le résultat de real_main:
  fprintf(outfile, "\tmovq\t%%rax, %s\n", call_registers[1]);
  // représente la string "%d\n"
  fprintf(outfile, "\tleaq	.LC0(%%rip), %s\n", call_registers[0]);
  fprintf(outfile, "\tcall printf@PLT\n");
  fprintf(outfile,
    "\tleave\n"
    "\tret\n");
}

/**
 * Generates intel x64 assembly from TAC instructions
 */
void asm_generator (buffer_t *buffer, FILE *outfile)
{
  int arg_count = 0;
  int param_count = 0;
  bool is_main = false;
  bool was_main = false;
  bool main_created = false;
  fprintf(outfile, "\t.globl\tmain\n");
  asm_symbol_t *table = NULL;
  do {
    buf_lock(buffer);
    char next = buf_getchar(buffer);
    buf_rollback_and_unlock(buffer, 1);
    if (next != '\t') {
      if (was_main == true && is_main == false) {
        main_created = true;
        asm_program_arguments(buffer, outfile, arg_count);
      }
      was_main = is_main;
      asm_label(buffer, outfile, &arg_count, &is_main);
      continue;
    }

    char *lexem = lexer_getalphanum(buffer);
    if (!strcmp(lexem, "ADD_STACK"))
      asm_add_stack(buffer, outfile);

    else if (!strcmp(lexem, "DECL_LOCAL"))
      asm_decl_local(buffer, &table, outfile);

    else if (!strcmp(lexem, "LOAD_ARG"))
      asm_load_arg(buffer, &table, &arg_count, outfile);

    else if (!strcmp(lexem, "ASSIGN"))
      asm_binary_op(buffer, table, outfile, "movq");

    else if (!strcmp(lexem, "COMPARE"))
      asm_binary_op(buffer, table, outfile, "cmpq");

    else if (!strcmp(lexem, "PARAM"))
      asm_param(buffer, table, &param_count, outfile);

    else if (!strcmp(lexem, "CALL"))
      asm_call(buffer, table, &param_count, outfile);

    else if (!strcmp(lexem, "RETURN"))
      asm_return(buffer, table, outfile);

    else if (!strncmp(lexem, "JUMP", 4))
      asm_jump(buffer, lexem, outfile);

    else if (!strncmp(lexem, "tmp", 3))
      asm_arithmetic(buffer, table, lexem, outfile);

    lexer_assert_newline(buffer, 
        "asm_instruction: Instruction should end with a '\\n'. exiting.\n");
    free(lexem);
  } while (!buf_eof_strict(buffer));

  if (!main_created) {
    asm_program_arguments(buffer, outfile, arg_count);
  }
}


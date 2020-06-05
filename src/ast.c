#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "symbol.h"
#include "buffer.h"
#include "parser.h"
#include "ast.h"
#include "utils.h"

char *copy_name (char *name)
{
  size_t len = (strlen(name) + 1);
  char *out = malloc(sizeof(char) * len);
  strncpy(out, name, len);
  return out;
}

ast_t *ast_new_integer (long val)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_INTEGER;
  ast->integer = val;
  return ast;
}

ast_t *ast_new_binary (ast_binary_e op, ast_t *left, ast_t *right)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_BINARY;
  ast->binary.op = op;
  ast->binary.left = left;
  ast->binary.right = right;
  return ast;
}

ast_t *ast_new_variable (char *name, int type) {
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_VARIABLE;
  ast->var.name = copy_name(name);
  ast->var.type = type;
  return ast;
}

ast_t *ast_new_unary (char op, ast_t *operand)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_UNARY;
  ast->unary.op = op;
  ast->unary.operand = operand;
  return ast;
}

ast_t *ast_new_function (char *name, int return_type, ast_list_t *params, ast_list_t *stmts)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_FUNCTION;
  ast->function.name = copy_name(name);
  ast->function.return_type = return_type;
  ast->function.params = params;
  ast->function.stmts = stmts;
  return ast;
}

ast_t *ast_new_fncall (char *name, ast_list_t *args)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_FNCALL;
  ast->call.name = copy_name(name);
  ast->call.args = args;
  return ast;
}

ast_t *ast_new_comp_stmt (ast_list_t *stmts)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_COMPOUND_STATEMENT;
  ast->compound_stmt.stmts = stmts;
  return ast;
}

ast_t *ast_new_declaration (ast_t *lvalue, ast_t *rvalue)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_DECLARATION;
  ast->assignment.lvalue = lvalue;
  ast->assignment.rvalue = rvalue;
  return ast;
}

ast_t *ast_new_assignment (ast_t *lvalue, ast_t *rvalue)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_ASSIGNMENT;
  ast->assignment.lvalue = lvalue;
  ast->assignment.rvalue = rvalue;
  return ast;
}

ast_t *ast_new_condition (ast_t *condition, ast_t *valid, ast_t *invalid)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_CONDITION;
  ast->branch.condition = condition;
  ast->branch.valid = valid;
  ast->branch.invalid = invalid;
  return ast;
}

ast_t *ast_new_loop (ast_t *condition, ast_t *stmt)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_LOOP;
  ast->loop.condition = condition;
  ast->loop.stmt = stmt;
  return ast;
}

ast_t *ast_new_return (ast_t *expr)
{
  ast_t * ast = malloc(sizeof(ast_t));
  ast->type = AST_RETURN;
  ast->ret.expr = expr;
  return ast;
}

char *ast_get_var_type (ast_t *ast) {
  if (ast->type != AST_VARIABLE) return "";
  if (ast->var.type == AST_INTEGER) {
    return "entier";
  }
  return "";
}

char *ast_get_ret_type (ast_t *ast) {
  if (ast->type != AST_FUNCTION) return "";
  if (ast->function.return_type == AST_INTEGER) {
    return "entier";
  }
  if (ast->function.return_type == AST_VOID) {
    return "rien";
  }
  return "";
}

ast_list_t *ast_list_new_node (ast_t *elem)
{
  ast_list_t *node = malloc(sizeof(ast_list_t));
  node->elem = elem;
  node->next = NULL;
  return node;
}

ast_list_t *ast_list_add (ast_list_t **list, ast_t *elem)
{
  assert(elem != NULL);
  assert(list != NULL);

  while (*list)
    list = &(*list)->next;

  *list = ast_list_new_node(elem);
  return *list;
}

int ast_binary_priority (ast_t *ast)
{
  if (ast == NULL) return 0;
  if (ast->type != AST_BINARY) return 100;

  switch (ast->binary.op) {
    case AST_BIN_OR: return 10;
    case AST_BIN_AND: return 10;
    case AST_BIN_GTE: return 20;
    case AST_BIN_GT: return 20;
    case AST_BIN_LTE: return 20;
    case AST_BIN_LT: return 20;
    case AST_BIN_DIFF: return 20;
    case AST_BIN_EQ: return 20;
    case AST_BIN_PLUS: return 30;
    case AST_BIN_MINUS: return 30;
    case AST_BIN_MULT: return 40;
    case AST_BIN_DIV: return 40;
    default: return 0;
  }
}

char *ast_binary_to_string (ast_binary_e op)
{
  switch (op)
  {
  case AST_BIN_PLUS: return "+";
  case AST_BIN_MINUS: return "-";
  case AST_BIN_MULT: return "*";
  case AST_BIN_DIV: return "/";
  case AST_BIN_AND: return "ET";
  case AST_BIN_OR: return "OU";
  case AST_BIN_LT: return "<";
  case AST_BIN_LTE: return "<=";
  case AST_BIN_GT: return ">";
  case AST_BIN_GTE: return ">=";
  case AST_BIN_EQ: return "==";
  case AST_BIN_DIFF: return "!=";
  default:
    printf("unknown binary operator. exiting.\n");
    exit(1);
  }
}

bool ast_is_binary (ast_t *ast)
{
  if (ast->type == AST_BINARY) return true;
  return false;
}

void print_spaces(size_t n)
{
  for (size_t i = 0; i < n; i++) printf(" ");
}

static
int ast_print_ (ast_t *ast, size_t indent)
{
  if (!ast) {
    printf("x");
    return 0;
  }
  ast_list_t *curr = NULL;
  int written = 0;
  size_t newindent = 0;
  switch (ast->type)
  {
  case AST_VOID:
    printf("void");
    break;
  case AST_INTEGER:
    printf("%ld", ast->integer);
    break;
  case AST_BINARY:
    printf("%s %n\u2500 ", ast_binary_to_string(ast->binary.op), &written);
    ast_print_(ast->binary.left, indent + written + 2);
    printf("\n");
    print_spaces(indent + written);
    printf("\u2798 ");
    ast_print_(ast->binary.right, indent + written + 2);
    break;
  case AST_UNARY:
    break;
  case AST_FUNCTION:
    printf("function %s: %s%n\n", ast->function.name, ast_get_ret_type(ast), &written);
    curr = ast->function.params;
    while (curr) {
      print_spaces(indent);
      printf(" * ");
      ast_print_(curr->elem, 0);
      printf("\n");
      curr = curr->next;
    }

    curr = ast->function.stmts;
    while (curr) {
      print_spaces(indent + 3);
      ast_print_(curr->elem, indent + 3);
      printf("\n");
      curr = curr->next;
    }
    break;
  case AST_FNCALL:
    printf("%s (%n\n", ast->call.name, &written);
    curr = ast->call.args;
    while (curr) {
      print_spaces(indent + written);
      ast_print_(curr->elem, indent + written);
      printf(",\n");
      curr = curr->next;
    }
    print_spaces(indent);
    printf(")");
    break;
  case AST_VARIABLE:
    printf("%s: %s%n", ast->var.name, ast_get_var_type(ast), &written);
    break;
  case AST_CONDITION:
    printf("if (%n\n", &written);
    print_spaces(indent + written);
    ast_print_(ast->branch.condition, indent + written);
    printf("\n");
    print_spaces(indent);
    printf(")\n");
    ast_print_(ast->branch.valid, indent);
    if (ast->branch.invalid) {
      print_spaces(indent);
      printf("else ");
      ast_print_(ast->branch.invalid, indent);
    } else {
      printf("\n");
    }
    break;
  case AST_LOOP:
    printf("while (%n\n", &written);
    print_spaces(indent + written);
    ast_print_(ast->loop.condition, indent + written);
    printf("\n");
    print_spaces(indent);
    printf(")\n");
    ast_print_(ast->loop.stmt, indent);
    break;
  case AST_DECLARATION:
  case AST_ASSIGNMENT:
    newindent = ast_print_(ast->declaration.lvalue, indent + 3);
    if (ast->declaration.rvalue) {
      printf(" = ");
      ast_print_(ast->declaration.rvalue, newindent);
    }
    printf(";");
    break;
  case AST_COMPOUND_STATEMENT:
    print_spaces(indent);
    printf("{\n");
    curr = ast->compound_stmt.stmts;
    while (curr) {
      print_spaces(indent + 3);
      ast_print_(curr->elem, indent + 3);
      printf("\n");
      curr = curr->next;
    }
    print_spaces(indent);
    printf("}\n");
    break;
  case AST_RETURN:
    // print_spaces(indent);
    printf("return: %n", &written);
    ast_print_(ast->ret.expr, indent + written + 3);
    break;
  }
  return indent + written;
}

void ast_print (ast_t *ast)
{
  printf("\n");
  ast_print_(ast, 0);
  printf("\n");
}

void ast_print_binary_or_integer (ast_t *item)
{
  if (item->type == AST_INTEGER) {
    printf(COLOR_BLUE "%ld " COLOR_DEFAULT, item->integer);
  }
  else if (item->type == AST_BINARY) {
    printf(COLOR_GREEN "%s " COLOR_DEFAULT, ast_binary_to_string(item->binary.op));
  } else if (item->type == AST_VARIABLE) {
    printf(COLOR_RED "%s " COLOR_DEFAULT, item->var.name);
  }
}
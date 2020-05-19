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

ast_t *ast_new_integer (long val)
{
  ast_t *ast = malloc(sizeof(ast_t));
  ast->type = AST_INTEGER;
  ast->integer = val;
  return ast;
}

ast_t *ast_new_binary (ast_binary_e op, ast_t *left, ast_t *right)
{
  ast_t *ast = malloc(sizeof(ast_t));
  ast->type = AST_BINARY;
  ast->binary.op = op;
  ast->binary.left = left;
  ast->binary.right = right;
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

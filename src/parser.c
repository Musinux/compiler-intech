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

#define DEBUG true

extern symbol_t **pglobal_table;
extern ast_t **past;

int parse_var_type (buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "entier") == 0) {
    return AST_INTEGER;
  }
  printf("Expected a valid type. exiting.\n");
  exit(1);
}

/**
 * 
 * (entier a, entier b, entier c) => une liste d'ast_t contenue dans un ast_list_t
 */
ast_list_t *parse_parameters (buffer_t *buffer)
{
  ast_list_t *list = NULL;
  buf_skipblank(buffer);
  lexer_assert_openbrace(buffer, "Expected a '(' after function name. exiting.\n");
  
  for (;;) {
    int type = parse_var_type(buffer);

    buf_skipblank(buffer);
    char *var_name = lexer_getalphanum(buffer);
    buf_skipblank(buffer);
    
    ast_list_add(&list, ast_new_variable(var_name, type));

    char next = buf_getchar(buffer);
    if (next == ')') break;
    if (next != ',') {
      printf("Expected either a ',' or a ')'. exiting.\n");
      exit(1);
    }
  }
  return list;
}

int parse_return_type (buffer_t *buffer)
{
  buf_skipblank(buffer);
  lexer_assert_twopoints(buffer, "Expected ':' after function parameters");
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "entier") == 0) {
    return AST_INTEGER;
  }
  else if (strcmp(lexem, "rien") == 0) {
    return AST_VOID;
  }
  printf("Expected a valid type for a parameter. exiting.\n");
  exit(1);
}

bool parse_is_type (char *lexem)
{
  if (strcmp(lexem, "entier") != 0) { // si le mot-clé n'est pas "entier", on retourne faux
    return false;
  }
  return true;
}

ast_t *parse_expression (buffer_t *buffer)
{
  // TODO
  return NULL;
}

/**
 * entier a;
 * entier a = 2;
 */
ast_t *parse_declaration (buffer_t *buffer)
{
  int type = parse_var_type(buffer);
  buf_skipblank(buffer);
  char *var_name = lexer_getalphanum(buffer);
  if (var_name == NULL) {
    printf("Expected a variable name. exiting.\n");
    exit(1);
  }

  ast_t *var = ast_new_variable(var_name, type);
  buf_skipblank(buffer);
  char next = buf_getchar(buffer);
  if (next == ';') {
    
    return ast_new_declaration(var, NULL);
  }
  else if (next == '=') {
    ast_t *expression = parse_expression(buffer);
    return ast_new_declaration(var, expression);
  }
  printf("Expected either a ';' or a '='. exiting.\n");
  buf_print(buffer);
  exit(1);
}

ast_t *parse_statement (buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum_rollback(buffer);
  if (parse_is_type(lexem)) {
    // ceci est une déclaration de variable
    return parse_declaration(buffer);
  }
  // TODO:
  return NULL;
}

ast_list_t *parse_function_body (buffer_t *buffer)
{
  ast_list_t *stmts = NULL;
  buf_skipblank(buffer);
  lexer_assert_openbracket(buffer, "Function body should start with a '{'");
  char next;
  do {
    ast_t *statement = parse_statement(buffer);
    ast_list_add(&stmts, statement);
    buf_skipblank(buffer);
    next = buf_getchar_rollback(buffer);
  } while (next != '}');

  return stmts;
}

/**
 * exercice: cf slides: https://docs.google.com/presentation/d/1AgCeW0vBiNX23ALqHuSaxAneKvsteKdgaqWnyjlHTTA/edit#slide=id.g86e19090a1_0_527
 */
ast_t *parse_function (buffer_t *buffer)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum(buffer);
  if (strcmp(lexem, "fonction") != 0) {
    printf("Expected a 'fonction' keyword on global scope.exiting.\n");
    buf_print(buffer);
    exit(1);
  }
  buf_skipblank(buffer);
  // TODO
  char *name = lexer_getalphanum(buffer);
  
  ast_list_t *params = parse_parameters(buffer);
  int return_type = parse_return_type(buffer);
  ast_list_t *stmts = parse_function_body(buffer);

  return ast_new_function(name, return_type, params, stmts);
}

/**
 * This function generates ASTs for each global-scope function
 */
ast_list_t *parse (buffer_t *buffer)
{
  ast_t *function = parse_function(buffer);
  ast_print(function);

  if (DEBUG) printf("** end of file. **\n");
  return NULL;
}

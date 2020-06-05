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
ast_list_t *parse_parameters (buffer_t *buffer, symbol_t **table)
{
  ast_list_t *list = NULL;
  buf_skipblank(buffer);
  lexer_assert_openbrace(buffer, "Expected a '(' after function name. exiting.\n");
  
  for (;;) {
    int type = parse_var_type(buffer);

    buf_skipblank(buffer);
    char *var_name = lexer_getalphanum(buffer);
    buf_skipblank(buffer);

    if (sym_search(*table, var_name)) {
      printf("Redeclaration of parameter %s. exiting.\n", var_name);
      exit(1);
    }
    
    ast_t *ast_var = ast_new_variable(var_name, type);
    sym_add(table, sym_new(var_name, SYM_PARAM, ast_var));

    ast_list_add(&list, ast_var);

    char next = buf_getchar(buffer);
    if (next == ')') break;
    if (next != ',') {
      printf("Expected either a ',' or a ')'. exiting.\n");
      exit(1);
    }
  }
  return list;
}

int parse_return_type (buffer_t *buffer, symbol_t **table)
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

ast_t *parse_expression_next_symbol(buffer_t *buffer, symbol_t **table) {
  // cette fonction doit deviner, grâce à buf_getchar, ou lexer_getalphanum, ou lexer_getop (à coder)
  // quel est le type de symbole qui est actuellement à lire
  // elle doit retourner l'ast_t correspondant à ce symbole
  // il n'est pas exclu de devoir rajouter des paramètres à cette fonction permettant par exemple de lui indiquer quels sont les types d'ast qui sont
  // autorisés à ce moment de la lecture (ex: une opérande après avoir lu un opérateur, ex pas de fin d'expression juste après un opérateur, etc.)
  return NULL; // TODO
}

ast_t *pile_vers_arbre (mystack_t *pile)
{
  if (stack_isempty(*pile)) return NULL;

  ast_t *curr = stack_pop(pile);
  if (ast_is_binary(curr)) {
    if (!curr->binary.right) curr->binary.right = pile_vers_arbre(pile);
    if (!curr->binary.left) curr->binary.left = pile_vers_arbre(pile);
  }
  
  return curr;
}

ast_t *parse_expression (buffer_t *buffer, symbol_t **table)
{
  mystack_t pile = NULL;
  mystack_t sortie = NULL;
  bool end_of_expression = false;
  ast_t *last_popped = NULL;
  ast_t *b = NULL;
//   positionner le curseur i sur le début de la chaîne
//   empiler le caractère nul dans pile
  stack_push(&pile, NULL);
//   répéter indéfiniment:
  for (;;) {
//      si i pointe sur le caractère nul et le sommet de la pile contient également le caractère nul
    if (stack_top(pile)  == NULL && end_of_expression == true) {
//        retourner sortie
      return pile_vers_arbre(&sortie);
    } 
//      sinon
    else {
//        soient a le symbole en sommet de pile et b le symbole pointé par i
      ast_t *a = stack_top(pile);
      b = parse_expression_next_symbol(buffer, table);
//        si a a une priorité plus basse que b
      if (ast_binary_priority(a) < ast_binary_priority(b)) {
//          empiler b dans pile
        stack_push(&pile, b);
//          avancer i sur le symbole suivant
        // TODO: lire le prochain symbole
      }
//        sinon
      else {
//          répéter
        do {
//            dépiler pile et empiler la valeur dépilée dans sortie
          last_popped = stack_pop(&pile);
          stack_push(&sortie, last_popped);
//            jusqu’à ce que le symbole en sommet de pile aie une priorité plus basse que le symbole le plus récemment dépilé
        } while (ast_binary_priority(stack_top(pile)) >= ast_binary_priority(last_popped));
      }
    }
  }
  return NULL;
}

/**
 * entier a;
 * entier a = 2;
 */
ast_t *parse_declaration (buffer_t *buffer, symbol_t **table)
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
    ast_t *expression = parse_expression(buffer, table);
    return ast_new_declaration(var, expression);
  }
  printf("Expected either a ';' or a '='. exiting.\n");
  buf_print(buffer);
  exit(1);
}

ast_t *parse_statement (buffer_t *buffer, symbol_t **table)
{
  buf_skipblank(buffer);
  char *lexem = lexer_getalphanum_rollback(buffer);
  if (parse_is_type(lexem)) {
    // ceci est une déclaration de variable
    return parse_declaration(buffer, table);
  }
  // TODO:
  return NULL;
}

ast_list_t *parse_function_body (buffer_t *buffer, symbol_t **table)
{
  ast_list_t *stmts = NULL;
  buf_skipblank(buffer);
  lexer_assert_openbracket(buffer, "Function body should start with a '{'");
  char next;
  do {
    ast_t *statement = parse_statement(buffer, table);
    ast_list_add(&stmts, statement);
    buf_skipblank(buffer);
    next = buf_getchar_rollback(buffer);
  } while (next != '}');

  return stmts;
}

/**
 * exercice: cf slides: https://docs.google.com/presentation/d/1AgCeW0vBiNX23ALqHuSaxAneKvsteKdgaqWnyjlHTTA/edit#slide=id.g86e19090a1_0_527
 */
ast_t *parse_function (buffer_t *buffer, symbol_t **table)
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
  
  ast_list_t *params = parse_parameters(buffer, table);
  int return_type = parse_return_type(buffer, table);
  ast_list_t *stmts = parse_function_body(buffer, table);

  return ast_new_function(name, return_type, params, stmts);
}

/**
 * This function generates ASTs for each global-scope function
 */
ast_list_t *parse (buffer_t *buffer)
{
  symbol_t *table = NULL;
  ast_t *function = parse_function(buffer, &table);
  ast_print(function);

  if (DEBUG) printf("** end of file. **\n");
  return NULL;
}

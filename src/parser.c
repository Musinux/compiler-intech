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

/**
 * exercice: cf slides: https://docs.google.com/presentation/d/1AgCeW0vBiNX23ALqHuSaxAneKvsteKdgaqWnyjlHTTA/edit#slide=id.g86e19090a1_0_527
 */
ast_t *parse_function (buffer_t *buffer)
{
  // TODO
  char *name = NULL;
  
  // ast_list_t *params = parse_parameters(buffer);
  // int return_type = parse_return_type(buffer);
  // ast_list_t *stmts = parse_function_body(buffer);

  // return ast_new_function(name, return_type, params, stmts);
  return NULL;
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

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
 * This function generates ASTs for each global-scope function
 */
ast_list_t *parse (buffer_t *buffer)
{

  // TODO
  if (DEBUG) printf("** end of file. **\n");
  return NULL;
}

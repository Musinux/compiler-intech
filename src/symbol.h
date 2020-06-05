/* vim: set tabstop=4:softtabstop=4:shiftwidth=4 */
#ifndef SYMBOL_H
#define SYMBOL_H
#include <stdbool.h>
#include "ast.h"


typedef enum {
  SYM_FUNCTION,
  SYM_VAR,
  SYM_PARAM
} sym_type_t;

#define SYM_ISVAR(symbol) ((symbol)->type == SYM_VAR || (symbol)->type == SYM_PARAM)
#define SYM_ISFUN(symbol) ((symbol)->type == SYM_FUNCTION)

typedef struct symbol_t {
  char *name;
  sym_type_t type; // symbol type
  ast_t *attributes;
  size_t rel_pos;
  struct symbol_t *function_table;
  struct symbol_t *next;
} symbol_t;

symbol_t *sym_new_function (char *name, int type, ast_t *attributes, symbol_t *table);
symbol_t * sym_new (char *name, int type, ast_t *attributes);
void sym_delete (symbol_t * sym);
void sym_remove (symbol_t **table, symbol_t *sym);
void sym_add (symbol_t **table, symbol_t *sym);
void sym_print_list (symbol_t *table);
symbol_t * sym_search (symbol_t *table, char *name);

#endif /* ifndef SYMBOL_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "symbol.h"
#include "ast.h"
#include "utils.h"

int next_id = 0;


static char *copy_name (char *name)
{
  size_t len = strlen(name) + 1;
  char *out = malloc(sizeof(char) * len);
  strncpy(out, name, len);
  return out;
}

symbol_t *sym_new_function (char *name, int type, ast_t *attributes, symbol_t *table)
{
  symbol_t *sym = sym_new(name, type, attributes);
  sym->function_table = table;
  return sym;
}

symbol_t *sym_new (char *name, int type, ast_t *attributes)
{
  symbol_t *sym = malloc(sizeof(symbol_t));

  sym->name = copy_name(name);
  sym->type = type;
  sym->rel_pos = 0;
  sym->function_table = NULL;
  sym->attributes = attributes;
  sym->next = NULL;
  return sym;
}

void sym_delete (symbol_t * sym)
{
  if (!sym) return;
  free(sym->name);
  if (sym->attributes) // FIXME probably not the way to go
    free(sym->attributes);
  free(sym);
}

void sym_remove (symbol_t **table, symbol_t *sym)
{
  assert(sym);
  assert(table);
  symbol_t *curr = *table;
  symbol_t *prec = NULL;
  while (curr) {
    if (sym == curr) {
      if (!prec) *table = curr->next;
      else prec->next = curr->next;
      sym_delete(curr);
      break;
    }
    prec = curr;
    curr = curr->next;
  }
}

void sym_add (symbol_t **table, symbol_t *sym)
{
  assert(sym);
  assert(table);
  if (!*table) {
    *table = sym;
    return;
  }
  symbol_t *curr = *table;

  while (curr->next) curr = curr->next;
  curr->next = sym;
}

symbol_t * sym_search (symbol_t *table, char *name)
{
  while (table) {
    if (strcmp(table->name, name) == STREQUAL) {
      return table;
    }
    table = table->next;
  }
  return NULL;
}

char * sym_get_symbol_type (sym_type_t type)
{
  switch (type) {
    case SYM_FUNCTION: return "fonction";
    case SYM_VAR: return "variable";
    case SYM_PARAM: return "parametre";
    default: return "";
  }
}

void sym_print_list (symbol_t *table)
{
  while (table) {
    printf("  %s '" COLOR_GREEN "%s" COLOR_DEFAULT "' : "
        COLOR_BLUE "%s" COLOR_DEFAULT "\n",
        sym_get_symbol_type(table->type),
        table->name,
        ast_get_var_type(table->attributes));
    table = table->next;
  }
}

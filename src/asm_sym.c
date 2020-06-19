#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "asm_sym.h"

asm_symbol_t *asm_sym_new (long pos, char *name)
{
  asm_symbol_t *out = malloc(sizeof(asm_symbol_t));
  out->pos = pos;
  out->name = name;
  out->next = NULL;
  return out;
}

void asm_sym_delete (asm_symbol_t * sym)
{
  if (!sym) return;
  free(sym->name);
  free(sym);
}

void asm_sym_remove (asm_symbol_t **table, asm_symbol_t *sym)
{
  assert(sym);
  assert(table);
  asm_symbol_t *curr = *table;
  asm_symbol_t *prec = NULL;
  while (curr) {
    if (sym == curr) {
      if (!prec) *table = curr->next;
      else prec->next = curr->next;
      asm_sym_delete(curr);
      break;
    }
    prec = curr;
    curr = curr->next;
  }
}

void asm_sym_add (asm_symbol_t **table, asm_symbol_t *sym)
{
  assert(sym != NULL);
  assert(table != NULL);
  if (!*table) {
    *table = sym;
    return;
  }
  asm_symbol_t *curr = *table;

  while (curr->next) curr = curr->next;
  curr->next = sym;
}

asm_symbol_t * asm_sym_search (asm_symbol_t *table, char *name)
{
  assert(name != NULL);
  while (table) {
    if (!strcmp(table->name, name))
      return table;
    table = table->next;
  }
  return NULL;
}

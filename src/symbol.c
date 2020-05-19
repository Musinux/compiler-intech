#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "symbol.h"
#include "ast.h"
#include "utils.h"


char * sym_get_symbol_type (sym_type_e type)
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

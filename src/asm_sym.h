#ifndef ASM_SYM_H
#define ASM_SYM_H

typedef struct asm_symbol_t {
  unsigned int pos; // relative position on the stack
  char *name;
  struct asm_symbol_t *next;
} asm_symbol_t;

asm_symbol_t *asm_sym_new (long pos, char *name);
void asm_sym_delete (asm_symbol_t * sym);
void asm_sym_remove (asm_symbol_t **table, asm_symbol_t *sym);
void asm_sym_add (asm_symbol_t **table, asm_symbol_t *sym);
void asm_sym_print_list (asm_symbol_t *table);
asm_symbol_t * asm_sym_search (asm_symbol_t *table, char *name);

#endif /* ifndef ASM_SYM_H */

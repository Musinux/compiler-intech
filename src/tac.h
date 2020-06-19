#ifndef TAC_H
#define TAC_H
#include <stdio.h>

void tac_condition (ast_t *ast, symbol_t *table, FILE *outfile,
    char *iftrue, char *iffalse, ast_binary_e parent_cond);
void tac_statement (ast_t *ast, symbol_t *table, FILE *outfile);
char *tac_expression (ast_t *ast, symbol_t *table, FILE *outfile);
void tac_generator (ast_list_t *functions, FILE *outfile);

#endif /* ifndef TAC_H */

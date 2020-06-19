#ifndef PARSER_H
#define PARSER_H
#include "buffer.h"
#include "ast.h"
#include "symbol.h"
#include "stack.h"

ast_list_t *parse (buffer_t *buffer);


void *parse_abort (buffer_t *buffer, const char *msg);

int parse_return_type (buffer_t *buffer);
int parse_type (buffer_t *buffer);
bool parse_is_type (buffer_t *buffer, char *str);

ast_list_t *parse_parameters (buffer_t *buffer, symbol_t **table);
ast_list_t *parse_function_body (buffer_t *buffer, symbol_t *fct);
ast_list_t *parse_arguments (buffer_t *buffer, symbol_t **table, symbol_t *function);

ast_t *parse_function (buffer_t *buffer);
ast_t *parse_number (buffer_t *buffer);
ast_t *parse_known_symbol (buffer_t *buffer, symbol_t **table);
ast_t *parse_statement (buffer_t *buffer, symbol_t *fct);
ast_t *parse_expression (buffer_t *buffer, symbol_t **table);
ast_t *parse_binary_expression (buffer_t *buffer, symbol_t **table);
bool   parse_expression_end (buffer_t *buffer);
ast_t *parse_assignment (buffer_t *buffer, symbol_t **table, symbol_t *variable, char *lexem);
ast_t *parse_declaration (buffer_t *buffer, symbol_t **table);
void   parse_condition_start (buffer_t *buffer, symbol_t *fct, const char *expected, ast_t **condition, ast_t **valid);
ast_t *parse_loop (buffer_t *buffer, symbol_t *fct);
ast_t *parse_branch (buffer_t *buffer, symbol_t *fct);
ast_t *parse_compount_stmt (buffer_t *buffer, symbol_t *fct);
ast_t *parse_stack_to_ast (mystack_t *ordered);

#endif /* PARSER_H */

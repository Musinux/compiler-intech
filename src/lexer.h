#ifndef LEXER_H
#define LEXER_H
#include <stdbool.h>
#include "buffer.h"

bool isalphanum (char chr);
bool isnumber (char chr);
bool isop (char chr);

void lexer_assert_simplechar (buffer_t *buffer, char chr, char *msg);
void lexer_assert_twopoints (buffer_t *buffer, char *msg);
void lexer_assert_newline (buffer_t *buffer, char *msg);
void lexer_assert_semicolon (buffer_t *buffer, char *msg);
void lexer_assert_openbrace (buffer_t *buffer, char *msg);
void lexer_assert_closebrace (buffer_t *buffer, char *msg);
void lexer_assert_openbracket (buffer_t *buffer, char *msg);
void lexer_assert_closebracket (buffer_t *buffer, char *msg);
void lexer_assert_equalsign (buffer_t *buffer, char *msg);
void lexer_assert_blank (buffer_t *buffer, char *msg);

/** buf_getalphanum_rollback mallocs and rollbacks after read. **/
char *lexer_getalphanum_rollback (buffer_t *buffer);
/** buf_getalphanum mallocs and rollbacks only if nothing was parsed. **/
char *lexer_getalphanum (buffer_t *buffer);
/** buf_getop mallocs and rollbacks only if nothing was parsed. **/
char *lexer_getop (buffer_t *buffer);
/** buf_getnumber mallocs and rollbacks only if nothing was parsed. **/
char *lexer_getnumber (buffer_t *buffer);

#endif /* ifndef LEXER_H */

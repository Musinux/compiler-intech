#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"
#include "buffer.h"

bool isalphanum (char chr)
{
  return
    (chr >= 'a' && chr <= 'z') ||
    (chr >= 'A' && chr <= 'Z') ||
    (chr >= '0' && chr <= '9') ||
    chr == '_';
}

bool isnumber (char chr)
{
  return ((chr >= '0' && chr <= '9') || chr == '-');
}

bool isop (char chr)
{
  return 
    (chr == '=' || chr == '!' ||
     chr == '<' || chr == '>' ||
     chr == '+' || chr == '-' || chr == '*' || chr == '/' ||
     chr == 'E' || chr == 'T' ||
     chr == 'O' || chr == 'U'
    );
}

void lexer_assert_simplechar (buffer_t *buffer, char chr, char *msg)
{
  if (buf_getchar_after_blank(buffer) != chr) {
    printf("%s.\n", msg);
    buf_print(buffer);
    exit(1);
  }
}

void lexer_assert_blank (buffer_t *buffer, char *msg)
{
  if (!ISBLANK(buf_getchar(buffer))) {
    printf("%s.\n", msg);
    buf_print(buffer);
    exit(1);
  }
}

void lexer_assert_newline (buffer_t *buffer, char *msg)
{
  if (buf_getchar(buffer) != '\n') {
    printf("%s.\n", msg);
    buf_print(buffer);
    exit(1);
  }
}

void lexer_assert_twopoints (buffer_t *buffer, char *msg)
{ lexer_assert_simplechar(buffer, ':', msg); }

void lexer_assert_semicolon (buffer_t *buffer, char *msg)
{ lexer_assert_simplechar(buffer, ';', msg); }

void lexer_assert_openbrace (buffer_t *buffer, char *msg)
{ lexer_assert_simplechar(buffer, '(', msg); }

void lexer_assert_closebrace (buffer_t *buffer, char *msg)
{ lexer_assert_simplechar(buffer, ')', msg); }

void lexer_assert_openbracket (buffer_t *buffer, char *msg)
{ lexer_assert_simplechar(buffer, '{', msg); }

void lexer_assert_closebracket (buffer_t *buffer, char *msg)
{ lexer_assert_simplechar(buffer, '}', msg); }

void lexer_assert_equalsign (buffer_t *buffer, char *msg)
{ lexer_assert_simplechar(buffer, '=', msg); }

static
char *lexer_get(buffer_t *buffer, char *lexem, size_t lexer_size,
    bool (*discriminator)(char))
{
  buf_skipblank(buffer);
  size_t count = 0;
  char *out = NULL;
  char chr;
  bool waslocked = true;
  if (!buffer->islocked) {
    buf_lock(buffer);
    waslocked = false;
  }

  while (count < lexer_size && discriminator(chr = buf_getchar(buffer)))
    lexem[count++] = chr;

  if (count < lexer_size)
    buf_rollback(buffer, 1);

  if (count > 0) {
    lexem[count] = '\0';
    out = malloc(sizeof(char) * (count + 1));
    strncpy(out, lexem, count + 1);
  }
  if (!waslocked)
    buf_unlock(buffer);
  return out;
}

char *lexer_getnumber (buffer_t *buffer)
{
  char lexem[LEXEM_SIZE + 1];
  return lexer_get(buffer, lexem, LEXEM_SIZE, isnumber);
}

char *lexer_getalphanum (buffer_t *buffer)
{
  char lexem[LEXEM_SIZE + 1];
  return lexer_get(buffer, lexem, LEXEM_SIZE, isalphanum);
}

char *lexer_getalphanum_rollback (buffer_t *buffer)
{
  char lexem[LEXEM_SIZE + 1];
  buf_lock(buffer);
  char *out = lexer_get(buffer, lexem, LEXEM_SIZE, isalphanum);
  if (out) buf_rollback(buffer, strlen(out));
  buf_unlock(buffer);
  return out;
}

char *lexer_getop (buffer_t *buffer)
{
  char lexem[3] = "";
  return lexer_get(buffer, lexem, 2, isop);
}

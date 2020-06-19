/* vim: set tabstop=4:softtabstop=4:shiftwidth=4 */
#ifndef BUFFER_H
#include <stdio.h>
#include <stdbool.h>

#define LEXEM_SIZE 60
#define BUF_SIZE 1024
#define ISBLANK(chr) ((chr) == ' ' || (chr) == '\n' || (chr) == '\t')

typedef struct buffer_t {
  char content[BUF_SIZE];
  FILE *fd;
  size_t it; // iterator
  size_t end;
  size_t avail; // available
  size_t lock; // can't overwrite any char between lock and end, even if 'it' is moved
  size_t bytesread;
  size_t bytesreadsincelock;
  size_t currchar;
  bool islocked;
  bool eof;
} buffer_t;

void buf_print (buffer_t *buffer);
void buf_init (buffer_t *buffer, FILE *fd);
bool buf_eof (buffer_t *buffer);
bool buf_eof_strict (buffer_t *buffer);
char buf_getchar (buffer_t *buffer);
char buf_getchar_after_blank (buffer_t *buffer);
char buf_getchar_rollback (buffer_t *buffer);
void buf_getnchar (buffer_t *buffer, char *out, size_t n);
void buf_forward (buffer_t *buffer, size_t n);
void buf_rollback (buffer_t *buffer, size_t n);
void buf_lock (buffer_t *buffer);
void buf_unlock (buffer_t *buffer);
void buf_rollback_and_unlock (buffer_t *buffer, size_t n);
size_t buf_skipblank (buffer_t *buffer);

#define BUFFER_H
#endif /* BUFFER_H */

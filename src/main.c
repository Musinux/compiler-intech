#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "symbol.h"
#include "buffer.h"
#include "ast.h"
#include "parser.h"
#include "utils.h"
#include "tac.h"
#include "asm.h"

symbol_t *global_table = NULL;
symbol_t **pglobal_table = &global_table;

void help (char *prg_name)
{
  printf("Usage: %s <file.intech>\n", prg_name);
}

int suffix (const char *buffer, const char *endswith) {
  size_t b_len = strlen(buffer);
  size_t e_len = strlen(endswith);
  if (b_len < e_len) return 1;
  return strcmp(&buffer[b_len - e_len], endswith);
}

void print_functions (ast_list_t *functions)
{
  printf("\n\n\n");
  ast_list_t *curr = functions;
  while (curr) {
    ast_print(curr->elem);
    printf("\n");
    curr = curr->next;
  }
}

char *create_asm_filename (const char *filename)
{
  size_t tac_filename_size = sizeof(char) * strlen(filename) + sizeof(".S");
  char *tac_filename = malloc(tac_filename_size);
  snprintf(tac_filename, tac_filename_size, "%s.S", filename);
  return tac_filename;
}

char *create_interm_filename (const char *filename)
{
  size_t tac_filename_size = sizeof(char) * strlen(filename) + sizeof(".interm");
  char *tac_filename = malloc(tac_filename_size);
  snprintf(tac_filename, tac_filename_size, "%s.interm", filename);
  return tac_filename;
}

char *launch_tac_generator (ast_list_t *functions, const char *filename)
{
  char *tac_filename = create_interm_filename(filename);

  FILE *tac_file = fopen(tac_filename, "w");
  tac_generator(functions, tac_file);
  fclose(tac_file);
  return tac_filename;
}

ast_list_t *launch_parser (const char *filename)
{
  buffer_t buffer;
  FILE *input = fopen(filename, "r");
  buf_init(&buffer, input);

  ast_list_t *functions = parse(&buffer);
  
  fclose(input);

  print_functions(functions);
  return functions;
}

char *launch_asm_generator (const char *tac_filename, const char *filename)
{
  buffer_t buffer;
  char *asm_filename = create_asm_filename(filename);
  FILE *input = fopen(tac_filename, "r");
  FILE *output = fopen(asm_filename, "w");
  buf_init(&buffer, input);

  asm_generator(&buffer, output);
  return asm_filename;
}

int main (int argc, char **argv)
{
  if (argc != 2) {
    help(argv[0]);
    printf("Not enough arguments.\n");
    exit(1);
  }

  const char *filename = argv[1];

  if (suffix(filename, ".intech") != 0) {
    printf("File does not terminate with .intech\n");
    exit(1);
  }

  printf("Lecture du fichier " COLOR_GREEN "%s" COLOR_DEFAULT "\n", filename);

  ast_list_t *functions = launch_parser(filename);
  char *tac_filename = launch_tac_generator(functions, filename);
  char *asm_filename = launch_asm_generator(tac_filename, filename);

  free(tac_filename);
  free(asm_filename);
  return 0;
}

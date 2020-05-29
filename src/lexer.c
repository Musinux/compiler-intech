#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"
#include "buffer.h"

bool isnbr (char chr)
{
  return (chr >= '0' && chr <= '9') || chr == '-';
}

// prend 1 caractère en paramètre
// et retourne vrai s'il correspond à un chiffre, une lettre,
// ou un underscore
bool isalphanum (char chr)
{
  // TODO: commencer par implémenter isalphanum
  if (chr == '_' ||
      (chr >= '0' && chr <= '9') ||
      (chr >= 'A' && chr <= 'Z') ||
      (chr >= 'a' && chr <= 'z')
    ) {
     return true;
  }
  // faire le reste
  return false;
}
void lexer_assert_openbracket (buffer_t *buffer, char *msg)
{
  char next = buf_getchar(buffer);
  if (next != '{') {
    printf("%s. exiting.\n", msg);
    buf_print(buffer);
    exit(1);
  }
}

void lexer_assert_openbrace (buffer_t *buffer, char *msg)
{
  char next = buf_getchar(buffer);
  if (next != '(') {
    printf("%s. exiting.\n", msg);
    buf_print(buffer);
    exit(1);
  }
}

void lexer_assert_twopoints (buffer_t *buffer, char *msg)
{
  char next = buf_getchar(buffer);
  if (next != ':') {
    printf("%s. exiting.\n", msg);
    buf_print(buffer);
    exit(1);
  }
}

/**
 * Notre objectif est de lire le maximum de caractères
 * tant que ceux-ci correspondent aux possibilités suivantes:
 *  * a-z
 *  * A-Z 
 *  * 0-9
 *  * _
 * Lorsqu'on a trouvé une suite de caractères qui matchent
 * allouer de la mémoire (malloc) et copier ces caractères dedans
 * retourner l'espace alloué
 * 
 * Attention: si aucun caractère ne matchait, retourner NULL
 * 
 */
char *lexer_getalphanum (buffer_t *buffer)
{
  // boucle qui s'arrête lorsque l'on tombe sur un caractère
  // qui ne correspond pas à isalphanum()
  /*
    tant que (buf_getchar(buffer) correspond à un caractère autorisé
      par isalphanum(), alors
        continuer
      sinon s'arrêter

    créer une chaîne de caractères contenant les caractères qu'on
       vient de lire
    et retourner cette chaine
  */
  char save[LEXEM_SIZE] = "";
  size_t count = 0;
  buf_lock(buffer);
  do {
    save[count] = buf_getchar(buffer);
    count++;
  } while (count < LEXEM_SIZE && isalphanum(save[count - 1]));

  buf_rollback(buffer, 1);
  buf_unlock(buffer);

  if (count == LEXEM_SIZE) {
    printf("Error parsing identifier: identifier too long!. exiting\n");
    exit(1); // arrêt brutal du programme
  }

  char *out = malloc(sizeof(char) * count);
  save[count - 1] = '\0';
  strncpy(out, save, count);
  
  return out;
}

char *lexer_getalphanum_rollback (buffer_t *buffer)
{
  char save[LEXEM_SIZE] = "";
  size_t count = 0;
  buf_lock(buffer);
  do {
    save[count] = buf_getchar(buffer);
    count++;
  } while (count < LEXEM_SIZE && isalphanum(save[count - 1]));

  buf_rollback(buffer, count);
  buf_unlock(buffer);

  if (count == LEXEM_SIZE) {
    printf("Error parsing identifier: identifier too long!. exiting\n");
    exit(1); // arrêt brutal du programme
  }

  char *out = malloc(sizeof(char) * count);
  save[count - 1] = '\0';
  strncpy(out, save, count);
  
  return out;
}

long lexer_getnumber (buffer_t *buffer)
{
  char save[LEXEM_SIZE] = "";
  size_t count = 0;
  buf_lock(buffer);
  do {
    save[count] = buf_getchar(buffer);
    count++;
  } while (count < LEXEM_SIZE && isnbr(save[count - 1]));

  buf_rollback(buffer, 1);
  buf_unlock(buffer);

  if (count == LEXEM_SIZE) {
    printf("Error parsing identifier: identifier too long!. exiting\n");
    exit(1); // arrêt brutal du programme
  }

  save[count - 1] = '\0';
  long out = strtol(save, NULL, 10);
  
  return out;
}
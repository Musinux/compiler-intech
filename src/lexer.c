#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"
#include "buffer.h"
// prend 1 caractère en paramètre
// et retourne vrai s'il correspond à un chiffre, une lettre,
// ou un underscore
bool isalphanum (char chr)
{
  // TODO: commencer par implémenter isalphanum
  if (chr == '_') return true;
  // faire le reste
  return false;
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
char *lexer_getalphanum_rollback (buffer_t *buffer)
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
 return NULL;
}
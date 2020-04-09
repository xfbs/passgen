#pragma
#include "passgen/array.h"
#include <stdio.h>

typedef struct passgen_wordlist_t {
  /* array of arrays (by size) of words */
  passgen_array_t words;
} passgen_wordlist_t;

/* initialise a wordlist. must be called before it can be used.
 * this scans the file for words and adds them as file offets into the wordlist
 * */
void passgen_wordlist_init(passgen_wordlist_t *wordlist, FILE *file);

size_t passgen_wordlist_get(
    passgen_wordlist_t *wordlist, size_t length, size_t number);

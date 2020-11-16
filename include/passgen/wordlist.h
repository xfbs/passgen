#pragma once
#include <stdio.h>
#include "passgen/random.h"

typedef struct passgen_wordlist_t {
  // entire wordlist
  char *data;
  // size of wordlist (in bytes)
  size_t size;

  // words in wordlist, separately
  char **words;
  // count of words in wordlist
  size_t count;
} passgen_wordlist_t;

/* initialise a wordlist. must be called before it can be used.
 * this scans the file for words and adds them as file offets into the wordlist
 * */
void passgen_wordlist_load(passgen_wordlist_t *wordlist, FILE *file);

const char *passgen_wordlist_random(
    passgen_wordlist_t *wordlist,
    passgen_random_t *random);

/* read a whole wordlist in from a file */
void passgen_wordlist_read(passgen_wordlist_t *wordlist, FILE *file);

/* scan a read wordlist for words */
void passgen_wordlist_scan(passgen_wordlist_t *wordlist);

void passgen_wordlist_free(passgen_wordlist_t *wordlist);

#pragma once
#include "passgen/random.h"
#include "passgen/stack.h"
#include "passgen/markov.h"
#include <stdio.h>

typedef struct passgen_wordlist {
    // entire wordlist
    char *data;
    // size of wordlist (in bytes)
    size_t size;

    // words in wordlist, separately
    char **words;
    // count of words in wordlist
    size_t count;

    passgen_stack stack;
    passgen_markov markov;
} passgen_wordlist;

/* initialise a wordlist. must be called before it can be used.
 * this scans the file for words and adds them as file offets into the wordlist
 * */
void passgen_wordlist_load(passgen_wordlist *wordlist, FILE *file);

const char *
passgen_wordlist_random(passgen_wordlist *wordlist, passgen_random *random);

/* read a whole wordlist in from a file */
void passgen_wordlist_read(passgen_wordlist *wordlist, FILE *file);

/* scan a read wordlist for words */
void passgen_wordlist_scan(passgen_wordlist *wordlist);

void passgen_wordlist_free(passgen_wordlist *wordlist);

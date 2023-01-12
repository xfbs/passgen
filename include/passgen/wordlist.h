/// @file wordlist.h
///
/// Utilities for parsing wordlists, with markov chain generation.
#pragma once
#include "passgen/util/random.h"
#include "passgen/markov.h"
#include <stdio.h>

/// Data structure used to keep wordlists.
typedef struct passgen_wordlist {
    /// entire wordlist
    char *data;
    /// size of wordlist (in bytes)
    size_t size;

    /// words in wordlist, separately
    char **words;
    /// count of words in wordlist
    size_t count;

    passgen_markov markov;
} passgen_wordlist;

/// Initialise a wordlist, must be called before it can be used.
/// this scans the file for words and adds them as file offets into the wordlist
void passgen_wordlist_load(passgen_wordlist *wordlist, FILE *file, size_t markov_depth);

/// Generate random word from this wordlist.
const char *
passgen_wordlist_random(passgen_wordlist *wordlist, passgen_random *random);

/// Read a whole wordlist in from a file.
void passgen_wordlist_read(passgen_wordlist *wordlist, FILE *file);

/// Scan a read wordlist for words.
void passgen_wordlist_scan(passgen_wordlist *wordlist);

/// Release memory for wordlist.
void passgen_wordlist_free(passgen_wordlist *wordlist);

/// Get the wordcount of this wordlist
static inline size_t passgen_wordlist_count(passgen_wordlist *wordlist) {
    return wordlist->count;
}

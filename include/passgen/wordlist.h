/// @file wordlist.h
///
/// Utilities for parsing wordlists, with markov chain generation.
#pragma once
#include "passgen/util/random.h"
#include "passgen/markov.h"
#include <stdio.h>

/// Data structure used to keep wordlists.
typedef struct passgen_wordlist {
    /// Wordlist is parsed.
    bool parsed;

    /// Wordlist is parsed into markov chain.
    bool parsed_markov;

    /// Should close file when done reading.
    bool should_close_file;

    /// File of the wordlist
    FILE *file;

    /// Entire wordlist
    char *data;

    /// Size of wordlist (in bytes).
    size_t size;

    /// Words in wordlist, separately. These are pointers into @ref data.
    char **words;

    /// Count of words in wordlist.
    size_t count;

    /// Markov chain parsed from this wordlist.
    passgen_markov markov;
} passgen_wordlist;

/// Convenience function to initialize, open and parse a wordlist, including the markov chain.
void passgen_wordlist_load(passgen_wordlist *wordlist, FILE *file, size_t markov_length);

/// Initialize a wordlist with a file object.
void passgen_wordlist_init(passgen_wordlist *wordlist, FILE *file, size_t markov_length);

/// Load words from wordlist. After running this, the file object can be closed.
int passgen_wordlist_parse(passgen_wordlist *wordlist);

/// Generate markov chain from wordlist.
void passgen_wordlist_parse_markov(passgen_wordlist *wordlist);

/// Generate random word from this wordlist.
const char *
passgen_wordlist_random(passgen_wordlist *wordlist, passgen_random *random);

/// Read a whole wordlist in from a file.
int passgen_wordlist_read(passgen_wordlist *wordlist, FILE *file);

/// Scan a read wordlist for words.
void passgen_wordlist_scan(passgen_wordlist *wordlist);

/// Release memory for wordlist.
void passgen_wordlist_free(passgen_wordlist *wordlist);

/// Get the wordcount of this wordlist
size_t passgen_wordlist_count(passgen_wordlist *wordlist);

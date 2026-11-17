/// @file wordlist.h
///
/// Utilities for parsing wordlists, with markov chain generation.
#pragma once
#include "passgen/random.h"
#include "passgen/markov.h"
#include <passgen/error.h>
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

/// Read a wordlist from a file, and build the Markov chain from it. The
/// file is closed.
///
/// @memberof passgen_wordlist
/// @param error Set to what went wrong on failure, may be NULL
/// @returns @ref PASSGEN_OK, or what @ref passgen_wordlist_parse returns
passgen_status passgen_wordlist_load(
    passgen_wordlist *wordlist,
    passgen_error *error,
    FILE *file,
    size_t markov_length);

/// Initialize a wordlist with a file object, which it will close.
///
/// @memberof passgen_wordlist
void passgen_wordlist_init(passgen_wordlist *wordlist, FILE *file, size_t markov_length);

/// Load words from wordlist, and close the file.
///
/// If this fails, the wordlist is as it was before, and still has the file.
///
/// @memberof passgen_wordlist
/// @param error Set to what went wrong on failure, may be NULL
/// @returns @ref PASSGEN_OK, @ref PASSGEN_ERROR_IO if the file cannot be
///   read (the detail of the error is the reason), or
///   @ref PASSGEN_ERROR_OUT_OF_MEMORY
passgen_status
passgen_wordlist_parse(passgen_wordlist *wordlist, passgen_error *error);

/// Generate markov chain from wordlist, which has to be parsed.
///
/// @memberof passgen_wordlist
/// @param error Set to what went wrong on failure, may be NULL
/// @returns @ref PASSGEN_OK or @ref PASSGEN_ERROR_OUT_OF_MEMORY
passgen_status
passgen_wordlist_parse_markov(passgen_wordlist *wordlist, passgen_error *error);

/// Generate random word from this wordlist.
///
/// @memberof passgen_wordlist
const char *
passgen_wordlist_random(passgen_wordlist *wordlist, passgen_random *random);

/// Read the whole file of a wordlist into memory.
///
/// @memberof passgen_wordlist
passgen_status
passgen_wordlist_read(passgen_wordlist *wordlist, passgen_error *error);

/// Scan a read wordlist for words.
///
/// @memberof passgen_wordlist
passgen_status
passgen_wordlist_scan(passgen_wordlist *wordlist, passgen_error *error);

/// Release memory for wordlist.
///
/// @memberof passgen_wordlist
void passgen_wordlist_free(passgen_wordlist *wordlist);

/// Get the wordcount of this wordlist
///
/// @memberof passgen_wordlist
size_t passgen_wordlist_count(passgen_wordlist *wordlist);

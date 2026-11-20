/// @file wordlist.h
///
/// Utilities for parsing wordlists, with markov chain generation.
#pragma once
#include "passgen/markov.h"
#include "passgen/random.h"
#include <passgen/error.h>
#include <stdio.h>

/// Length of the hash of the file of a wordlist (BLAKE2b), in bytes.
#define PASSGEN_WORDLIST_HASH_SIZE 64

/// Data structure used to keep wordlists.
typedef struct passgen_wordlist {
    /// Wordlist is parsed.
    bool parsed;

    /// Which of the Markov chains are built: bit n is for the length n + 1.
    uint8_t parsed_markov;

    /// Should close file when done reading.
    bool should_close_file;

    /// File of the wordlist, until it is read. NULL if it was not opened
    /// yet, in which case it is opened from @ref path.
    FILE *file;

    /// Path of the file of the wordlist, which it is opened from when it is
    /// first read, or NULL if it was given a file. Owned by the wordlist.
    char *path;

    /// The start of the hash which the file has to have (BLAKE2b with 64
    /// bytes), in lowercase hex, or NULL if it is not checked. Owned by the
    /// wordlist.
    char *hash;

    /// Entire wordlist
    char *data;

    /// Size of wordlist (in bytes).
    size_t size;

    /// Words in wordlist, separately. These are pointers into @ref data.
    char **words;

    /// How many words there is room for in @ref words.
    size_t capacity;

    /// Count of words in wordlist.
    size_t count;

    /// Markov chains built from this wordlist, one for every length (the
    /// first is of length 1). They are built when they are first needed, see
    /// @ref passgen_wordlist_parse_markov.
    passgen_markov markov[PASSGEN_MARKOV_LENGTH_MAX];
} passgen_wordlist;

/// Read a wordlist from a file. The file is closed.
///
/// @memberof passgen_wordlist
/// @param error Set to what went wrong on failure, may be NULL
/// @returns @ref PASSGEN_OK, or what @ref passgen_wordlist_parse returns
passgen_status passgen_wordlist_load(
    passgen_wordlist *wordlist,
    passgen_error *error,
    FILE *file);

/// Initialize a wordlist with a file object, which it will close.
///
/// @memberof passgen_wordlist
void passgen_wordlist_init(passgen_wordlist *wordlist, FILE *file);

/// Initialize a wordlist with the path of its file, which is only opened
/// when it is read.
///
/// @memberof passgen_wordlist
/// @param path Path of the file, allocated with @ref passgen_alloc_string.
///   The wordlist takes ownership of it.
/// @param hash The start of the hash the file has to have, in lowercase hex,
///   allocated with @ref passgen_alloc_string, or NULL. The wordlist takes
///   ownership of it.
void passgen_wordlist_init_path(
    passgen_wordlist *wordlist,
    char *path,
    char *hash);

/// Load words from wordlist, and close the file. If the wordlist only has
/// the path of its file, it is opened first.
///
/// If this fails, the wordlist is as it was before, and still has the file
/// (if it could be opened).
///
/// @memberof passgen_wordlist
/// @param error Set to what went wrong on failure, may be NULL
/// @returns @ref PASSGEN_OK, @ref PASSGEN_ERROR_IO if the file cannot be
///   opened or read (the detail of the error is the reason),
///   @ref PASSGEN_ERROR_UTF8 if a word is not valid UTF-8 (the detail of the
///   error is its line), or @ref PASSGEN_ERROR_OUT_OF_MEMORY
passgen_status
passgen_wordlist_parse(passgen_wordlist *wordlist, passgen_error *error);

/// Build the Markov chain of a length from the wordlist, which has to be
/// parsed, unless it is built already.
///
/// If this fails, the Markov chain is empty, and this can be tried again.
///
/// @memberof passgen_wordlist
/// @param length Length of the chain, from 1 to @ref PASSGEN_MARKOV_LENGTH_MAX
/// @param error Set to what went wrong on failure, may be NULL
/// @returns @ref PASSGEN_OK or @ref PASSGEN_ERROR_OUT_OF_MEMORY
passgen_status passgen_wordlist_parse_markov(
    passgen_wordlist *wordlist,
    size_t length,
    passgen_error *error);

/// The Markov chain of a length, which has to be built with @ref
/// passgen_wordlist_parse_markov.
///
/// @memberof passgen_wordlist
static inline passgen_markov *
passgen_wordlist_markov(passgen_wordlist *wordlist, size_t length) {
    return &wordlist->markov[length - 1];
}

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

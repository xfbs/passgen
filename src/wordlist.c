#include "passgen/wordlist.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include "passgen/util/error.h"
#include "passgen/util/utf8.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALLOC_INITIAL  256
#define ALLOC_INCREASE 2

// Fails because of the file, with what the system has to say about it.
static passgen_status
passgen_wordlist_fail_io(passgen_error *error, int code, const char *fallback) {
    passgen_error_set(error, PASSGEN_ERROR_IO);
    passgen_error_detail_set(error, code ? strerror(code) : fallback);
    return PASSGEN_ERROR_IO;
}

void passgen_wordlist_init(passgen_wordlist *wordlist, FILE *file) {
    wordlist->parsed = false;
    wordlist->parsed_markov = 0;
    wordlist->file = file;
    wordlist->path = NULL;
    wordlist->hash = NULL;
    wordlist->should_close_file = true;
    wordlist->data = NULL;
    wordlist->size = 0;
    wordlist->words = NULL;
    wordlist->capacity = 0;
    wordlist->count = 0;
    for(size_t i = 0; i <= PASSGEN_MARKOV_LENGTH_MAX; i++) {
        passgen_markov_init(&wordlist->markov[i], i);
    }
}

void passgen_wordlist_init_path(
    passgen_wordlist *wordlist,
    char *path,
    char *hash) {
    passgen_wordlist_init(wordlist, NULL);
    wordlist->path = path;
    wordlist->hash = hash;
}

// Checks that what was read has the hash it has to have, if any.
static passgen_status
wordlist_hash_check(passgen_wordlist *wordlist, passgen_error *error) {
    if(!wordlist->hash) {
        return PASSGEN_OK;
    }
#ifdef PASSGEN_MONOCYPHER
    uint8_t hash[PASSGEN_WORDLIST_HASH_SIZE];
    passgen_blake2b(
        hash,
        sizeof(hash),
        (const uint8_t *) wordlist->data,
        wordlist->size);
    for(size_t i = 0; wordlist->hash[i]; i++) {
        uint8_t byte = hash[i / 2];
        char digit = "0123456789abcdef"[i % 2 ? byte & 15 : byte >> 4];
        if(digit != wordlist->hash[i]) {
            return passgen_error_set(error, PASSGEN_ERROR_HASH);
        }
    }
    return PASSGEN_OK;
#else
    // it is not kept then.
    (void) error;
    return PASSGEN_OK;
#endif
}

passgen_status
passgen_wordlist_parse(passgen_wordlist *wordlist, passgen_error *error) {
    passgen_assert(!wordlist->parsed);
    passgen_assert(wordlist->file || wordlist->path);

    // a wordlist which was given by its path is opened when it is needed.
    if(!wordlist->file) {
        errno = 0;
        wordlist->file = fopen(wordlist->path, "r");
        if(!wordlist->file) {
            return passgen_wordlist_fail_io(error, errno, "cannot open");
        }
    }

    passgen_status status = passgen_wordlist_read(wordlist, error);
    if(status == PASSGEN_OK) {
        status = wordlist_hash_check(wordlist, error);
    }
    if(status == PASSGEN_OK) {
        status = passgen_wordlist_scan(wordlist, error);
    }

    if(status != PASSGEN_OK) {
        // nothing is kept of a wordlist which was not read completely, as
        // words would be missing from it. the file is kept, so that this
        // fails in the same way when it is tried again.
        passgen_free(wordlist->words, wordlist->capacity * sizeof(char *));
        passgen_free(wordlist->data, wordlist->size + 1);
        wordlist->words = NULL;
        wordlist->data = NULL;
        wordlist->capacity = 0;
        wordlist->count = 0;
        wordlist->size = 0;
        return status;
    }

    // the file is not needed anymore.
    if(wordlist->should_close_file) {
        fclose(wordlist->file);
    }
    wordlist->file = NULL;
    wordlist->parsed = true;

    return PASSGEN_OK;
}

passgen_status passgen_wordlist_parse_markov(
    passgen_wordlist *wordlist,
    size_t length,
    passgen_error *error) {
    passgen_assert(wordlist->parsed);
    passgen_require(length <= PASSGEN_MARKOV_LENGTH_MAX);

    uint16_t bit = 1 << length;
    if(wordlist->parsed_markov & bit) {
        return PASSGEN_OK;
    }
    passgen_markov *markov = passgen_wordlist_markov(wordlist, length);

    // a word can never decode to more codepoints than it has bytes, so size
    // the buffer for the longest word in the list.
    size_t unicode_buffer_len = 1;
    for(size_t i = 0; i < wordlist->count; i++) {
        size_t word_length = strlen(wordlist->words[i]);
        if(word_length > unicode_buffer_len) {
            unicode_buffer_len = word_length;
        }
    }

    size_t unicode_buffer_size = unicode_buffer_len * sizeof(uint32_t);
    uint32_t *unicode_buffer = passgen_alloc(unicode_buffer_size);
    if(!unicode_buffer) {
        return passgen_error_set(error, PASSGEN_ERROR_OUT_OF_MEMORY);
    }

    passgen_status status = PASSGEN_OK;
    for(size_t i = 0; status == PASSGEN_OK && i < wordlist->count; i++) {
        uint32_t *unicode_buffer_pos = &unicode_buffer[0];
        const uint8_t *word = (const uint8_t *) wordlist->words[i];
        size_t word_length = strlen((const char *) word);

        // decode utf8
        int ret = passgen_utf8_decode(
            &unicode_buffer_pos,
            unicode_buffer_len,
            NULL,
            &word,
            word_length);

        // reading the wordlist made sure that every word is valid UTF-8.
        passgen_assert(ret == PASSGEN_UTF8_SUCCESS);

        // add word to markov chain
        status = passgen_markov_add(
            markov,
            unicode_buffer,
            unicode_buffer_pos - &unicode_buffer[0],
            1);
    }

    passgen_free(unicode_buffer, unicode_buffer_size);

    if(status != PASSGEN_OK) {
        // a chain with only some of the words in it would be generated from
        // as if nothing happened. it starts over when this is tried again.
        passgen_markov_free(markov);
        passgen_markov_init(markov, length);
        return passgen_error_set(error, status);
    }

    wordlist->parsed_markov |= bit;

    return PASSGEN_OK;
}

passgen_status passgen_wordlist_load(
    passgen_wordlist *wordlist,
    passgen_error *error,
    FILE *file) {
    passgen_wordlist_init(wordlist, file);
    return passgen_wordlist_parse(wordlist, error);
}

passgen_status
passgen_wordlist_read(passgen_wordlist *wordlist, passgen_error *error) {
    FILE *file = wordlist->file;

    // get size of file. this fails for anything that is not seekable (such as
    // a pipe).
    errno = 0;
    if(0 != fseek(file, 0, SEEK_END)) {
        return passgen_wordlist_fail_io(error, errno, "cannot seek");
    }
    long size = ftell(file);
    if(size < 0 || 0 != fseek(file, 0, SEEK_SET)) {
        return passgen_wordlist_fail_io(error, errno, "cannot seek");
    }
    wordlist->size = (size_t) size;

    // allocate buffer to hold entire file and read in (plus zero termination)
    wordlist->data = passgen_alloc(wordlist->size + 1);
    if(!wordlist->data) {
        return passgen_error_set(error, PASSGEN_ERROR_OUT_OF_MEMORY);
    }

    // read in entire wordlist
    errno = 0;
    size_t read = fread(wordlist->data, 1, wordlist->size, file);
    if(read != wordlist->size) {
        return passgen_wordlist_fail_io(
            error,
            ferror(file) ? errno : 0,
            "unexpected end of file");
    }

    // null-terminate wordlist
    wordlist->data[wordlist->size] = 0;

    return PASSGEN_OK;
}

// Whether a word is valid UTF-8. It is decoded in chunks, so that words of
// any length work.
static bool wordlist_word_valid(const char *word) {
    const uint8_t *pos = (const uint8_t *) word;
    const uint8_t *end = pos + strlen(word);
    uint32_t codepoints[64];
    int ret;
    do {
        uint32_t *codepoints_pos = &codepoints[0];
        ret = passgen_utf8_decode(&codepoints_pos, 64, NULL, &pos, end - pos);
    } while(ret > PASSGEN_UTF8_SUCCESS);
    return ret == PASSGEN_UTF8_SUCCESS;
}

passgen_status
passgen_wordlist_scan(passgen_wordlist *wordlist, passgen_error *error) {
    wordlist->words = passgen_alloc(sizeof(char *) * ALLOC_INITIAL);
    wordlist->count = 0;
    if(!wordlist->words) {
        return passgen_error_set(error, PASSGEN_ERROR_OUT_OF_MEMORY);
    }
    wordlist->capacity = ALLOC_INITIAL;

    // every line is a word, which ends where the line does (the data ends
    // with a NUL character). a line can end in a carriage return and a
    // newline, as on Windows, and the carriage return is not part of the
    // word. blank lines are skipped.
    char *pos = wordlist->data;
    char *end = wordlist->data + wordlist->size;
    for(size_t line = 1; pos < end; line++) {
        char *token = pos;
        char *newline = memchr(pos, '\n', end - pos);
        if(newline) {
            *newline = 0;
            if(newline > token && newline[-1] == '\r') {
                newline[-1] = 0;
            }
            pos = newline + 1;
        } else {
            pos = end;
        }
        if(!*token) {
            continue;
        }

        // a word which is not valid UTF-8 is a mistake in the wordlist. it
        // is found when the wordlist is read, and not only when the word
        // happens to be chosen.
        if(!wordlist_word_valid(token)) {
            passgen_error_set(error, PASSGEN_ERROR_UTF8);
            char detail[32];
            snprintf(detail, sizeof(detail), "line %zu", line);
            passgen_error_detail_set(error, detail);
            return PASSGEN_ERROR_UTF8;
        }

        // expand array if needed
        if(wordlist->capacity == wordlist->count) {
            size_t capacity = wordlist->capacity * ALLOC_INCREASE;
            char **words = passgen_resize(
                wordlist->words,
                sizeof(char *) * wordlist->capacity,
                sizeof(char *) * capacity);
            if(!words) {
                // a wordlist with only some of its words would be chosen
                // from as if nothing happened, so this is an error.
                return passgen_error_set(error, PASSGEN_ERROR_OUT_OF_MEMORY);
            }
            wordlist->words = words;
            wordlist->capacity = capacity;
        }

        // save word
        wordlist->words[wordlist->count] = token;
        wordlist->count++;
    }

    // resize array finally (shrink it down to what is really needed)
    if(wordlist->count > 0 && wordlist->count < wordlist->capacity) {
        char **words = passgen_resize(
            wordlist->words,
            sizeof(char *) * wordlist->capacity,
            sizeof(char *) * wordlist->count);
        if(words) {
            wordlist->words = words;
            wordlist->capacity = wordlist->count;
        }
    }

    return PASSGEN_OK;
}

const char *
passgen_wordlist_random(passgen_wordlist *wordlist, passgen_random *random) {
    if(!wordlist->parsed || wordlist->count == 0) {
        return NULL;
    }
    // with a single word, there is nothing to choose from, so this takes no
    // randomness. words which are in the wordlist more than once are not
    // noticed, they are separate choices.
    if(wordlist->count == 1) {
        return wordlist->words[0];
    }

    size_t index = passgen_random_u64_max(random, wordlist->count);
    return wordlist->words[index];
}

void passgen_wordlist_free(passgen_wordlist *wordlist) {
    for(size_t i = 0; i <= PASSGEN_MARKOV_LENGTH_MAX; i++) {
        passgen_markov_free(&wordlist->markov[i]);
    }

    passgen_free(wordlist->words, wordlist->capacity * sizeof(char *));
    passgen_free(wordlist->data, wordlist->size + 1);

    if(wordlist->should_close_file && wordlist->file) {
        fclose(wordlist->file);
    }
    passgen_free_string(wordlist->path);
    passgen_free_string(wordlist->hash);

    PASSGEN_CLEAR(wordlist);
}

size_t passgen_wordlist_count(passgen_wordlist *wordlist) {
    return wordlist->count;
}

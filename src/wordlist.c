#include "passgen/wordlist.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/util/alloc.h"
#include "passgen/util/utf8.h"
#include "passgen/util/error.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "passgen/util/clear.h"

#define ALLOC_INITIAL  256
#define ALLOC_INCREASE 2

void passgen_wordlist_init(
    passgen_wordlist *wordlist,
    FILE *file,
    size_t markov_length) {
    wordlist->parsed = false;
    wordlist->parsed_markov = false;
    wordlist->file = file;
    wordlist->should_close_file = true;
    wordlist->data = NULL;
    wordlist->size = 0;
    wordlist->words = NULL;
    wordlist->capacity = 0;
    wordlist->count = 0;
    passgen_markov_init(&wordlist->markov, markov_length);
}

passgen_status
passgen_wordlist_parse(passgen_wordlist *wordlist, passgen_error *error) {
    passgen_assert(!wordlist->parsed);

    passgen_status status = passgen_wordlist_read(wordlist, error);
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

passgen_status
passgen_wordlist_parse_markov(passgen_wordlist *wordlist, passgen_error *error) {
    passgen_assert(wordlist->parsed);
    passgen_assert(!wordlist->parsed_markov);

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

        // skip words that are not valid UTF-8.
        if(ret != PASSGEN_UTF8_SUCCESS) {
            continue;
        }

        // add word to markov chain
        status = passgen_markov_add(
            &wordlist->markov,
            unicode_buffer,
            unicode_buffer_pos - &unicode_buffer[0],
            1);
    }

    passgen_free(unicode_buffer, unicode_buffer_size);

    if(status != PASSGEN_OK) {
        // a chain with only some of the words in it would be generated from
        // as if nothing happened. it starts over when this is tried again.
        size_t level = wordlist->markov.level;
        passgen_markov_free(&wordlist->markov);
        passgen_markov_init(&wordlist->markov, level);
        return passgen_error_set(error, status);
    }

    wordlist->parsed_markov = true;

    return PASSGEN_OK;
}

passgen_status passgen_wordlist_load(
    passgen_wordlist *wordlist,
    passgen_error *error,
    FILE *file,
    size_t markov_length) {
    passgen_wordlist_init(wordlist, file, markov_length);
    passgen_status status = passgen_wordlist_parse(wordlist, error);
    if(status == PASSGEN_OK) {
        status = passgen_wordlist_parse_markov(wordlist, error);
    }
    return status;
}

// Fails because of the file, with what the system has to say about it.
static passgen_status
passgen_wordlist_fail_io(passgen_error *error, int code, const char *fallback) {
    passgen_error_set(error, PASSGEN_ERROR_IO);
    passgen_error_detail_set(error, code ? strerror(code) : fallback);
    return PASSGEN_ERROR_IO;
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

passgen_status
passgen_wordlist_scan(passgen_wordlist *wordlist, passgen_error *error) {
    char *state = NULL, *token;

    wordlist->words = passgen_alloc(sizeof(char *) * ALLOC_INITIAL);
    wordlist->count = 0;
    if(!wordlist->words) {
        return passgen_error_set(error, PASSGEN_ERROR_OUT_OF_MEMORY);
    }
    wordlist->capacity = ALLOC_INITIAL;

    token = strtok_r(wordlist->data, "\n", &state);
    while(token) {
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

        // get next word
        token = strtok_r(NULL, "\n", &state);
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
    size_t index = passgen_random_u64_max(random, wordlist->count);
    return wordlist->words[index];
}

void passgen_wordlist_free(passgen_wordlist *wordlist) {
    passgen_markov_free(&wordlist->markov);

    passgen_free(wordlist->words, wordlist->capacity * sizeof(char *));
    passgen_free(wordlist->data, wordlist->size + 1);

    if(wordlist->should_close_file && wordlist->file) {
        fclose(wordlist->file);
    }

    PASSGEN_CLEAR(wordlist);
}

size_t passgen_wordlist_count(passgen_wordlist *wordlist) {
    return wordlist->count;
}

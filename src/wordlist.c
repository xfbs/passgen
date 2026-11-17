#include "passgen/wordlist.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/util/utf8.h"
#include "try.h"
#include <stdlib.h>
#include <string.h>

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
    wordlist->count = 0;
    passgen_markov_init(&wordlist->markov, markov_length);
}

int passgen_wordlist_parse(passgen_wordlist *wordlist) {
    passgen_assert(!wordlist->parsed);
    wordlist->parsed = true;
    try(passgen_wordlist_read(wordlist, wordlist->file));
    wordlist->file = NULL;
    passgen_wordlist_scan(wordlist);
    return 0;
}

void passgen_wordlist_parse_markov(passgen_wordlist *wordlist) {
    passgen_assert(!wordlist->parsed_markov);
    wordlist->parsed_markov = true;

    // a word can never decode to more codepoints than it has bytes, so size
    // the buffer for the longest word in the list.
    size_t unicode_buffer_len = 1;
    for(size_t i = 0; i < wordlist->count; i++) {
        size_t word_length = strlen(wordlist->words[i]);
        if(word_length > unicode_buffer_len) {
            unicode_buffer_len = word_length;
        }
    }

    uint32_t *unicode_buffer = malloc(unicode_buffer_len * sizeof(uint32_t));
    if(!unicode_buffer) {
        return;
    }

    for(size_t i = 0; i < wordlist->count; i++) {
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
        passgen_markov_add(
            &wordlist->markov,
            unicode_buffer,
            unicode_buffer_pos - &unicode_buffer[0],
            1);
    }

    free(unicode_buffer);
}

void passgen_wordlist_load(
    passgen_wordlist *wordlist,
    FILE *file,
    size_t markov_length) {
    passgen_wordlist_init(wordlist, file, markov_length);
    if(0 == passgen_wordlist_parse(wordlist)) {
        passgen_wordlist_parse_markov(wordlist);
    }
}

int passgen_wordlist_read(passgen_wordlist *wordlist, FILE *file) {
    // get size of file. this fails for anything that is not seekable (such as
    // a pipe).
    if(0 != fseek(file, 0, SEEK_END)) {
        return 1;
    }
    long size = ftell(file);
    if(size < 0 || 0 != fseek(file, 0, SEEK_SET)) {
        return 1;
    }
    wordlist->size = (size_t) size;

    // allocate buffer to hold entire file and read in (plus zero termination)
    wordlist->data = malloc(wordlist->size + 1);
    if(!wordlist->data) {
        return 1;
    }

    // read in entire wordlist
    size_t read = fread(wordlist->data, 1, wordlist->size, file);
    if(read != wordlist->size) {
        return 1;
    }

    // null-terminate wordlist
    wordlist->data[wordlist->size] = 0;

    if(wordlist->should_close_file) {
        fclose(file);
    }

    return 0;
}

void passgen_wordlist_scan(passgen_wordlist *wordlist) {
    char *state = NULL, *token;

    size_t capacity = ALLOC_INITIAL;
    wordlist->words = malloc(sizeof(char *) * capacity);
    wordlist->count = 0;
    if(!wordlist->words) {
        return;
    }

    token = strtok_r(wordlist->data, "\n", &state);
    while(token) {
        // expand array if needed
        if(capacity == wordlist->count) {
            capacity *= ALLOC_INCREASE;
            char **words = realloc(wordlist->words, sizeof(char *) * capacity);
            if(!words) {
                // out of memory: keep the words collected so far.
                return;
            }
            wordlist->words = words;
        }

        // save word
        wordlist->words[wordlist->count] = token;
        wordlist->count++;

        // get next word
        token = strtok_r(NULL, "\n", &state);
    }

    // resize array finally (shrink it down to what is really needed)
    if(wordlist->count > 0) {
        char **words =
            realloc(wordlist->words, sizeof(char *) * wordlist->count);
        if(words) {
            wordlist->words = words;
        }
    }
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

    free(wordlist->words);
    free(wordlist->data);

    if(wordlist->should_close_file && wordlist->file) {
        fclose(wordlist->file);
    }

    PASSGEN_CLEAR(wordlist);
}

size_t passgen_wordlist_count(passgen_wordlist *wordlist) {
    return wordlist->count;
}

#include "passgen/wordlist.h"
#include "passgen/assert.h"
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

    uint32_t unicode_buffer[256];
    size_t unicode_buffer_len;
    for(size_t i = 0; i < wordlist->count; i++) {
        unicode_buffer_len = 0;
        size_t word_length = strlen(wordlist->words[i]);
        size_t word_pos = 0;
        passgen_utf8_decode(
            unicode_buffer,
            256,
            &unicode_buffer_len,
            NULL,
            (const unsigned char *) wordlist->words[i],
            word_length,
            &word_pos);
        passgen_markov_add(
            &wordlist->markov,
            unicode_buffer,
            unicode_buffer_len,
            1);
    }
}

void passgen_wordlist_load(
    passgen_wordlist *wordlist,
    FILE *file,
    size_t markov_length) {
    passgen_wordlist_init(wordlist, file, markov_length);
    passgen_wordlist_parse(wordlist);
    passgen_wordlist_parse_markov(wordlist);
}

int passgen_wordlist_read(passgen_wordlist *wordlist, FILE *file) {
    // get size of file
    fseek(file, 0, SEEK_END);
    wordlist->size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // allocate buffer to hold entire file and read in (plus zero termination)
    wordlist->data = malloc(wordlist->size + 1);
    passgen_assert(wordlist->data);

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

    token = strtok_r(wordlist->data, "\n", &state);
    while(token) {
        // expand array if needed
        if(capacity == wordlist->count) {
            capacity *= ALLOC_INCREASE;
            wordlist->words =
                realloc(wordlist->words, sizeof(char *) * capacity);
        }

        // save word
        wordlist->words[wordlist->count] = token;
        wordlist->count++;

        // get next word
        token = strtok_r(NULL, "\n", &state);
    }

    // resize array finally (shrink it down to what is really needed)
    wordlist->words =
        realloc(wordlist->words, sizeof(char *) * wordlist->count);
}

const char *
passgen_wordlist_random(passgen_wordlist *wordlist, passgen_random *random) {
    if(!wordlist->parsed) {
        return NULL;
    }
    size_t index = passgen_random_u64_max(random, wordlist->count);
    return wordlist->words[index];
}

void passgen_wordlist_free(passgen_wordlist *wordlist) {
    passgen_markov_free(&wordlist->markov);

    if(wordlist->parsed) {
        free(wordlist->words);
        free(wordlist->data);
    }

    if(wordlist->should_close_file && wordlist->file) {
        fclose(wordlist->file);
    }
}

size_t passgen_wordlist_count(passgen_wordlist *wordlist) {
    return wordlist->count;
}

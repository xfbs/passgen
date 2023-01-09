#include "passgen/wordlist.h"
#include "passgen/assert.h"
#include "passgen/util/utf8.h"
#include <stdlib.h>
#include <string.h>

#define ALLOC_INITIAL  256
#define ALLOC_INCREASE 2

void passgen_wordlist_load(passgen_wordlist *wordlist, FILE *file) {
    passgen_markov_init(&wordlist->markov, 3);
    passgen_stack_init(&wordlist->stack, sizeof(uint32_t *));

    uint8_t buffer[1024];
    size_t buffer_len = 0;
    size_t buffer_pos = 0;
    uint32_t utf8_buffer[1024];
    size_t utf8_buffer_len = 0;
    size_t utf8_buffer_pos = 0;

    while(!feof(file)) {
        buffer_len += fread(&buffer[buffer_len], 1, 1024 - buffer_len, file);
        passgen_utf8_decode(
            utf8_buffer,
            1024,
            &utf8_buffer_len,
            NULL,
            buffer,
            buffer_len,
            &buffer_pos);

        // reset input buffer
        if(buffer_pos < buffer_len) {
            memmove(buffer, &buffer[buffer_pos], buffer_len - buffer_pos);
        }
        buffer_len = buffer_len - buffer_pos;
        buffer_pos = 0;

        while(utf8_buffer_pos < utf8_buffer_len) {
            bool success = false;
            for(size_t i = 0; (utf8_buffer_pos + i) < utf8_buffer_len; i++) {
                if(utf8_buffer[utf8_buffer_pos + i] == '\n') {
                    passgen_markov_add(
                        &wordlist->markov,
                        &utf8_buffer[utf8_buffer_pos],
                        i,
                        1);
                    uint32_t *word = calloc(i + 1, sizeof(uint32_t));
                    memcpy(
                        word,
                        &utf8_buffer[utf8_buffer_pos],
                        i * sizeof(uint32_t));
                    passgen_stack_push(&wordlist->stack, &word);
                    utf8_buffer_pos += i + 1;
                    success = true;
                    break;
                }
            }
            if(!success) {
                break;
            }
        }

        if(utf8_buffer_pos < utf8_buffer_len) {
            memmove(
                utf8_buffer,
                &utf8_buffer[utf8_buffer_pos],
                (utf8_buffer_len - utf8_buffer_pos) * sizeof(uint32_t));
        }
        utf8_buffer_len = utf8_buffer_len - utf8_buffer_pos;
        utf8_buffer_pos = 0;
    }

    passgen_wordlist_read(wordlist, file);
    passgen_wordlist_scan(wordlist);
}

void passgen_wordlist_read(passgen_wordlist *wordlist, FILE *file) {
    // get size of file
    fseek(file, 0, SEEK_END);
    wordlist->size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // allocate buffer to hold entire file and read in (plus zero termination)
    wordlist->data = malloc(wordlist->size + 1);
    passgen_assert(wordlist->data);

    // read in entire wordlist
    size_t read = fread(wordlist->data, 1, wordlist->size, file);
    passgen_assert(wordlist->size == read);

    // null-terminate wordlist
    wordlist->data[wordlist->size] = 0;
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
    size_t index = passgen_random_u64_max(random, wordlist->count);
    return wordlist->words[index];
}

static void free_word(void *element) {
    uint32_t **word = element;
    free(*word);
}

void passgen_wordlist_free(passgen_wordlist *wordlist) {
    passgen_markov_free(&wordlist->markov);
    passgen_stack_foreach(&wordlist->stack, free_word);
    passgen_stack_free(&wordlist->stack);
    free(wordlist->words);
    free(wordlist->data);
}

#include "passgen/wordlist.h"

#include <stdlib.h>
#include <string.h>
#include "passgen/assert.h"

#define ALLOC_INITIAL 256
#define ALLOC_INCREASE 2

void passgen_wordlist_load(passgen_wordlist_t *wordlist, FILE *file) {
  passgen_wordlist_read(wordlist, file);
  passgen_wordlist_scan(wordlist);
}

void passgen_wordlist_read(passgen_wordlist_t *wordlist, FILE *file) {
  // get size of file
  fseek(file, 0, SEEK_END);
  wordlist->size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // allocate buffer to hold entire file and read in
  wordlist->data = malloc(wordlist->size);
  assert(wordlist->data);

  // read in entire wordlist
  assert(wordlist->size == fread(wordlist->data, 1, wordlist->size, file));
}

void passgen_wordlist_scan(passgen_wordlist_t *wordlist) {
  char *state = NULL, *token;

  size_t capacity = ALLOC_INITIAL;
  wordlist->words = malloc(sizeof(char *) * capacity);
  wordlist->count = 0;

  token = strtok_r(wordlist->data, "\n", &state);
  while(token) {
    // expand array if needed
    if(capacity == wordlist->count) {
      capacity *= ALLOC_INCREASE;
      wordlist->words = realloc(wordlist->words, sizeof(char *) * capacity);
    }

    // save word
    wordlist->words[wordlist->count] = token;
    wordlist->count++;

    // get next word
    token = strtok_r(NULL, "\n", &state);
  }

  // resize array finally (shrink it down to what is really needed)
  wordlist->words = realloc(wordlist->words, sizeof(char *) * wordlist->count);
}

const char *passgen_wordlist_random(
    passgen_wordlist_t *wordlist,
    passgen_random_t *random) {
  size_t index = passgen_random_u64_max(random, wordlist->count);
  return wordlist->words[index];
}

void passgen_wordlist_free(passgen_wordlist_t *wordlist) {
  free(wordlist->words);
  free(wordlist->data);
}

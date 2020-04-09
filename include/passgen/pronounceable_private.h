#pragma once
#include "passgen/random.h"
#include <stddef.h>
#include <stdint.h>

struct markov0 {
  int32_t codepoint;
  size_t frequency;
};

struct markov1 {
  int32_t codepoint;
  size_t frequency_sum;
  const struct markov0 *list;
  size_t list_len;
};

struct markov2 {
  int32_t codepoint;
  const struct markov1 *list;
  size_t list_len;
};

struct markov {
  const struct markov2 *list;
  size_t list_len;
};

size_t passgen_pronounceable2(
    const struct markov *list, random_t *rand, int32_t *buf, size_t len);

/* given a markov chain and two codepoints, choose the next codepoint. */
const struct markov0 *passgen_pronounceable_choose(
    const struct markov *list, random_t *rand, int32_t a, int32_t b);

/* use a binary search to find a codepoint in a markov2 chain. */
const struct markov1 *
passgen_pronounceable_find1(const struct markov2 *list, int32_t codepoint);

/* use binary search to find a codepoint in a markov chain. returns NULL on
 * error. */
const struct markov2 *
passgen_pronounceable_find2(const struct markov *list, int32_t codepoint);

/* given a final markov chain list, find the right codepoint. */
const struct markov0 *
passgen_pronounceable_find(const struct markov1 *list, size_t choice);

/* list of symbols of all markov chains */
extern const struct markov passgen_pronounceable_english;
extern const struct markov passgen_pronounceable_latin;

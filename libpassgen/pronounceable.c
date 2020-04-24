#include "passgen/pronounceable.h"

#include <stdlib.h>
#include <string.h>

#include "passgen/assert.h"
#include "passgen/pronounceable_private.h"

const struct passgen_markov *markov_pronounceable_all[] = {
    &passgen_pronounceable_english,
    &passgen_pronounceable_german,
    &passgen_pronounceable_latin,
    NULL};

passgen_pronounceable_map_t passgen_pronounceable_map[] = {
    {"english", PASSGEN_PRONOUNCEABLE_ENGLISH},
    {"german", PASSGEN_PRONOUNCEABLE_GERMAN},
    {"latin", PASSGEN_PRONOUNCEABLE_LATIN},
    {NULL, 0},
};

enum passgen_pronounceable_type
passgen_pronounceable_lookup(size_t length, const char *name) {
  for(size_t i = 0; i < PASSGEN_PRONOUNCEABLE_LAST; i++) {
    if(0 == strncmp(passgen_pronounceable_map[i].name, name, length)) {
      return passgen_pronounceable_map[i].type;
    }
  }

  return PASSGEN_PRONOUNCEABLE_LAST;
}

const struct passgen_markov0 *passgen_pronounceable_choose(
    const struct passgen_markov *list,
    passgen_random_t *rand,
    int32_t p1,
    int32_t p2) {
  const struct passgen_markov2 *list2 = passgen_pronounceable_find2(list, p1);
  if(!list2) {
    return NULL;
  }

  const struct passgen_markov1 *list1 = passgen_pronounceable_find1(list2, p2);
  if(!list1) {
    return NULL;
  }

  size_t choices = list1->frequency_sum;
  size_t nchoice = passgen_random_u64_max(rand, choices);

  return passgen_pronounceable_find(list1, nchoice);
}

size_t passgen_pronounceable2(
    const struct passgen_markov *list,
    passgen_random_t *rand,
    int32_t *buf,
    size_t len) {
  int32_t p1 = 0, p2 = 0;
  size_t pos = 0;

  do {
    const struct passgen_markov0 *choice =
        passgen_pronounceable_choose(list, rand, p1, p2);
    assert(choice);

    /* save codepoint */
    if(choice->codepoint) {
      buf[pos] = choice->codepoint;
    }
    pos += 1;

    p1 = p2;
    p2 = choice->codepoint;
  } while(p2 && pos < len);

  return pos - 1;
}

size_t passgen_pronounceable(
    enum passgen_pronounceable_type type,
    passgen_random_t *rand,
    int32_t *buf,
    size_t len) {
  assert(type < PASSGEN_PRONOUNCEABLE_LAST);

  /* get list */
  const struct passgen_markov *list = markov_pronounceable_all[type];
  assert(list);

  return passgen_pronounceable2(list, rand, buf, len);
}

size_t passgen_pronounceable_len(
    enum passgen_pronounceable_type type,
    passgen_random_t *rand,
    int32_t *buf,
    size_t min,
    size_t max,
    size_t tries) {
  assert(type < PASSGEN_PRONOUNCEABLE_LAST);

  /* get list */
  const struct passgen_markov *list = markov_pronounceable_all[type];
  assert(list);

  /* try tries times (or infinite times if tries is 0) to fill buffer up with
   * a word. return 0 on success. */
  for(size_t i = 0; !tries || i < tries; i++) {
    int32_t p1 = 0, p2 = 0;

    /* position in the output buffer and codepoints is not the same thing. */
    size_t pos = 0;

    do {
      const struct passgen_markov0 *choice;
      choice = passgen_pronounceable_choose(list, rand, p1, p2);
      assert(choice);

      /* advante to next pair of previour */
      p1 = p2;
      p2 = choice->codepoint;

      /* make sure we don't write too much. */
      if(pos == max) {
        pos++;
        break;
      }

      /* save codepoint in buffer */
      buf[pos++] = choice->codepoint;
    } while(p2);

    /* if this isn't finished, or is too short, skip */
    if(p2 || pos <= min) {
      continue;
    }

    return pos - 1;
  }

  return 0;
}

static int markov1_find(const void *key_p, const void *item_p) {
  const int32_t *key = key_p;
  const struct passgen_markov1 *item = item_p;

  if(*key < item->codepoint) {
    return -1;
  } else if(*key > item->codepoint) {
    return 1;
  } else {
    return 0;
  }
}

const struct passgen_markov1 *
passgen_pronounceable_find1(const struct passgen_markov2 *list, int32_t codepoint) {
  const struct passgen_markov1 *result;

  result = bsearch(
      &codepoint,
      list->list,
      list->list_len,
      sizeof(struct passgen_markov1),
      markov1_find);

  return result;
}

static int markov_find(const void *key_p, const void *item_p) {
  const int32_t *key = key_p;
  const struct passgen_markov2 *item = item_p;

  if(*key < item->codepoint) {
    return -1;
  } else if(*key > item->codepoint) {
    return 1;
  } else {
    return 0;
  }
}

const struct passgen_markov2 *
passgen_pronounceable_find2(const struct passgen_markov *list, int32_t codepoint) {
  const struct passgen_markov2 *result;

  result = bsearch(
      &codepoint,
      list->list,
      list->list_len,
      sizeof(struct passgen_markov2),
      markov_find);

  return result;
}

const struct passgen_markov0 *
passgen_pronounceable_find(const struct passgen_markov1 *list, size_t choice) {
  for(size_t i = 0; i < list->list_len; i++) {
    if(choice < list->list[i].frequency) {
      return &list->list[i];
    }

    choice -= list->list[i].frequency;
  }

  return NULL;
}

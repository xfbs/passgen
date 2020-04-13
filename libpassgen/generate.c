#include "passgen/assert.h"
#include "passgen/debug.h"
#include "passgen/generate.h"
#include "passgen/data/range.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/group.h"
#include "passgen/container/stack/segment_item.h"

struct fillpos {
  char *buffer;
  size_t cur;
  size_t len;
};

static struct pattern_env pattern_env_default = {
    .find_complexity = false,
    .pronounceable_limit = 1000,
    .pronounceable_type = PASSGEN_PRONOUNCEABLE_ENGLISH,
};

static int pattern_random_write_buffer(void *data, int32_t codepoint) {
  struct fillpos *fillpos = data;

  if(fillpos->cur == fillpos->len) {
    return -1;
  }

  // TODO: utf8 generation
  fillpos->buffer[fillpos->cur] = codepoint;
  fillpos->cur++;

  return 0;
}

size_t pattern2_random_fill(
    struct passgen_pattern *pattern,
    random_t *rand,
    struct pattern_env *env,
    char *buffer,
    size_t len) {
  struct fillpos fillpos = {
      .buffer = buffer,
      .len = len,
      .cur = 0,
  };

  int ret =
      pattern2_random(pattern, rand, env, &fillpos, pattern_random_write_buffer);

  if(0 != ret) {
    return 0;
  }

  return fillpos.cur;
}

static size_t pattern_random_repeat(
    random_t *rand,
    struct pattern_env *env,
    struct passgen_pattern_repeat *repeat) {
  size_t difference = repeat->max - repeat->min;

  // if there is no difference to pick, just return here
  if(0 == difference) {
    return repeat->min;
  }

  // get random number to choose from the range
  size_t choice = random_uint64_max(rand, difference + 1);

  // keep track of complexity
  if(env->find_complexity) {
    env->complexity *= difference + 1;
  }

  return repeat->min + choice;
}

static inline int pattern_random_ranges(
    struct passgen_pattern_set *ranges,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  size_t reps = pattern_random_repeat(rand, env, &ranges->repeat);

  // compute number of possible codepoints
  // TODO: generate this on the fly.
  size_t possible = ranges->choices_list[ranges->items.len - 1];

  assert(possible != 0);

  // repeat chose rep size times
  for(size_t i = 0; i < reps; i++) {
    size_t choice = random_uint64_max(rand, possible);

    // keep track of complexity
    if(env->find_complexity) {
      env->complexity *= possible;
    }

    // locate choice in list of choices.
    // TODO: binary search.
    size_t num;
    for(num = 0; num < ranges->items.len; num++) {
      if(choice < ranges->choices_list[num]) {
        break;
      }
    }

    assert(num != ranges->items.len);

    /* adjust choice to be relative offset */
    if(num) {
      choice -= ranges->choices_list[num - 1];
    }

    struct passgen_pattern_range *range =
        passgen_pattern_range_stack_get(&ranges->items, num);

    int ret = func(data, range->start + choice);

    if(0 != ret) {
      return ret;
    }
  }

  return 0;
}

static inline int pattern_random_character(
    struct passgen_pattern_char *character,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  size_t reps = pattern_random_repeat(rand, env, &character->repeat);

  for(size_t i = 0; i < reps; i++) {
    int ret = func(data, character->codepoint);

    if(0 != ret) {
      return ret;
    }
  }

  return 0;
}

static inline int pattern_random_special_pronounceable(
    struct passgen_pattern_special *special,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  /* limit length to 64, that should be plenty. */
  size_t max = special->length.max;
  if(64 < max) max = 64;

  int32_t buffer[max];

  (void) max;
  (void) buffer;
  (void) rand;
  (void) env;
  (void) data;
  (void) func;
  // TODO: fixme

  /* TODO: get tries and default from env! */
  /*
  size_t count = passgen_pronounceable_len(
      special->data.pronounceable,
      rand,
      buffer,
      special->length.min,
      max,
      env->pronounceable_limit);
      */

  /* TODO error handling */
  /*
  if(!count) {
    return -2;
  }

  for(size_t i = 0; i < count; i++) {
    int ret = func(data, buffer[i]);

    if(0 != ret) {
      return ret;
    }
  }
  */

  return 0;
}

static inline int pattern_random_special_wordlist(
    struct passgen_pattern_special *special,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  // TODO: implement
  (void) special;
  (void) rand;
  (void) env;
  (void) data;
  (void) func;

  return 0;
}

static inline int pattern_random_special(
    struct passgen_pattern_special *special,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  switch(special->kind) {
    case PASSGEN_PATTERN_SPECIAL_PRONOUNCABLE:
      return pattern_random_special_pronounceable(
          special,
          rand,
          env,
          data,
          func);
      break;
    case PASSGEN_PATTERN_SPECIAL_WORDLIST:
      return pattern_random_special_wordlist(special, rand, env, data, func);
      break;
    default: assert(false); break;
  }
  return 0;
}

static inline int pattern_random_group(
    struct passgen_pattern_group *group,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func);

static inline int pattern_random_segment(
    struct passgen_pattern_item *segment,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  switch(segment->kind) {
    case PASSGEN_PATTERN_SET:
      return pattern_random_ranges(&segment->data.set, rand, env, data, func);
    case PASSGEN_PATTERN_CHAR:
      return pattern_random_character(
          &segment->data.character,
          rand,
          env,
          data,
          func);
    case PASSGEN_PATTERN_SPECIAL:
      return pattern_random_special(
          &segment->data.special,
          rand,
          env,
          data,
          func);
    case PASSGEN_PATTERN_GROUP:
      return pattern_random_group(&segment->data.group, rand, env, data, func);
  }

  // unreachable
  return 0;
}

static inline int pattern_random_segments(
    struct passgen_pattern_segment *segments,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  for(size_t i = 0; i < segments->items.len; i++) {
    struct passgen_pattern_item *segment;
    segment = passgen_pattern_item_stack_get(&segments->items, i);

    int ret = pattern_random_segment(segment, rand, env, data, func);

    if(0 != ret) {
      return ret;
    }
  }

  return 0;
}

static inline int pattern_random_group(
    struct passgen_pattern_group *group,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  // choose random number of repetitions
  size_t reps = pattern_random_repeat(rand, env, &group->repeat);

  for(size_t r = 0; r < reps; r++) {
    // choose random segment from segments
    size_t segment = random_uint64_max(rand, group->segments.len);

    // keep track of complexity
    if(env->find_complexity) {
      env->complexity *= group->segments.len;
    }

    // get segment from array
    struct passgen_pattern_segment *segments;
    segments = passgen_pattern_segment_stack_get(
        &group->segments,
        segment);

    int ret = pattern_random_segments(segments, rand, env, data, func);

    if(0 != ret) {
      return ret;
    }
  }

  return 0;
}

inline int pattern2_random(
    struct passgen_pattern *pattern,
    random_t *rand,
    struct pattern_env *env,
    void *data,
    pattern_random_cb *func) {
  /* use default env if none was supplied. this should be relatively sane. */
  if(!env) {
    env = &pattern_env_default;
  }

  if(env->find_complexity) {
    env->complexity = 1;
  }

  return pattern_random_group(&pattern->group, rand, env, data, func);
}

#include "passgen/assert.h"
#include "passgen/debug.h"
#include "passgen/pattern.h"
#include "passgen/pronounceable.h"

struct fillpos {
  char *buffer;
  size_t cur;
  size_t len;
};

static pattern_env_t pattern_env_default = {
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

size_t pattern_random_fill(
    pattern_t *pattern,
    random_t *rand,
    pattern_env_t *env,
    char *buffer,
    size_t len) {
  struct fillpos fillpos = {
      .buffer = buffer,
      .len = len,
      .cur = 0,
  };

  int ret =
      pattern_random(pattern, rand, env, &fillpos, pattern_random_write_buffer);

  if(0 != ret) {
    return 0;
  }

  return fillpos.cur;
}

static size_t pattern_random_repeat(
    random_t *rand,
    pattern_env_t *env,
    pattern_repeat_t *repeat) {
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
    pattern_ranges_t *ranges,
    random_t *rand,
    pattern_env_t *env,
    void *data,
    pattern_random_cb *func) {
  size_t reps = pattern_random_repeat(rand, env, &ranges->repeat);

  // compute number of possible codepoints
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

    pattern_range_t *range =
        passgen_array_get(&ranges->items, sizeof(pattern_range_t), num);

    int ret = func(data, range->start + choice);

    if(0 != ret) {
      return ret;
    }
  }

  return 0;
}

static inline int pattern_random_character(
    pattern_char_t *character,
    random_t *rand,
    pattern_env_t *env,
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
    pattern_special_t *special,
    random_t *rand,
    pattern_env_t *env,
    void *data,
    pattern_random_cb *func) {
  /* limit length to 64, that should be plenty. */
  size_t max = special->length.max;
  if(64 < max) max = 64;

  int32_t buffer[max];

  /* TODO: get tries and default from env! */
  size_t count = passgen_pronounceable_len(
      special->data.pronounceable,
      rand,
      buffer,
      special->length.min,
      max,
      env->pronounceable_limit);

  /* TODO error handling */
  if(!count) {
    return -2;
  }

  for(size_t i = 0; i < count; i++) {
    int ret = func(data, buffer[i]);

    if(0 != ret) {
      return ret;
    }
  }

  return 0;
}

static inline int pattern_random_special_wordlist(
    pattern_special_t *special,
    random_t *rand,
    pattern_env_t *env,
    void *data,
    pattern_random_cb *func) {
  return 0;
}

static inline int pattern_random_special(
    pattern_special_t *special,
    random_t *rand,
    pattern_env_t *env,
    void *data,
    pattern_random_cb *func) {
  switch(special->kind) {
    case PATTERN_SPECIAL_PRONOUNCABLE:
      return pattern_random_special_pronounceable(
          special,
          rand,
          env,
          data,
          func);
      break;
    case PATTERN_SPECIAL_WORDLIST:
      return pattern_random_special_wordlist(special, rand, env, data, func);
      break;
    default: assert(false); break;
  }
  return 0;
}

static inline int pattern_random_group(
    pattern_group_t *group,
    random_t *rand,
    pattern_env_t *env,
    void *data,
    pattern_random_cb *func);

static inline int pattern_random_segment(
    pattern_segment_t *segment,
    random_t *rand,
    pattern_env_t *env,
    void *data,
    pattern_random_cb *func) {
  switch(segment->kind) {
    case PATTERN_RANGE:
      return pattern_random_ranges(&segment->data.range, rand, env, data, func);
    case PATTERN_CHAR:
      return pattern_random_character(
          &segment->data.character,
          rand,
          env,
          data,
          func);
    case PATTERN_SPECIAL:
      return pattern_random_special(
          &segment->data.special,
          rand,
          env,
          data,
          func);
    case PATTERN_GROUP:
      return pattern_random_group(&segment->data.group, rand, env, data, func);
  }

  // unreachable
  return 0;
}

static inline int pattern_random_segments(
    pattern_segments_t *segments,
    random_t *rand,
    pattern_env_t *env,
    void *data,
    pattern_random_cb *func) {
  for(size_t i = 0; i < segments->items.len; i++) {
    pattern_segment_t *segment;
    segment = passgen_array_get(&segments->items, sizeof(pattern_segment_t), i);

    int ret = pattern_random_segment(segment, rand, env, data, func);

    if(0 != ret) {
      return ret;
    }
  }

  return 0;
}

static inline int pattern_random_group(
    pattern_group_t *group,
    random_t *rand,
    pattern_env_t *env,
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
    pattern_segments_t *segments;
    segments = passgen_array_get(
        &group->segments,
        sizeof(pattern_segments_t),
        segment);

    int ret = pattern_random_segments(segments, rand, env, data, func);

    if(0 != ret) {
      return ret;
    }
  }

  return 0;
}

inline int pattern_random(
    pattern_t *pattern,
    random_t *rand,
    pattern_env_t *env,
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

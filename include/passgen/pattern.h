/// @file pattern.h
/// @author Patrick M. Elsen
///
/// Methods for parsing a regex-subset used to generate custom data.

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "memory.h"
#include "passgen/data/array.h"
#include "passgen/pronounceable.h"
#include "random.h"
#include "token_old.h"
#include "unicode.h"

enum pattern_kind {
  PATTERN_RANGE,
  PATTERN_CHAR,
  PATTERN_SPECIAL,
  PATTERN_GROUP,
};

enum pattern_special_kind {
  PATTERN_SPECIAL_PRONOUNCABLE,
  PATTERN_SPECIAL_WORDLIST,
};

struct pattern;
struct pattern_segments;
struct pattern_segment;
struct pattern_char;
struct pattern_special;
struct pattern_group;
struct pattern_ranges;
struct pattern_range;

struct pattern_substring {
  size_t offset;
  size_t length;
};

/// Represents how many times a pattern must be repeated.
struct pattern_repeat {
  size_t min;
  size_t max;
};

/// Represents a sequence that is copied verbatim.
struct pattern_char {
  struct pattern_substring pos;
  int32_t codepoint;
  struct pattern_repeat repeat;
};

/// Range of possible characters.
struct pattern_range {
  struct pattern_substring pos;

  int32_t start;
  int32_t end;

  size_t multiplier;
};

/// Range. Represents something like `[a-z012]{3}`.
struct pattern_ranges {
  struct pattern_substring pos;

  // array of range_items
  passgen_array_t items;

  // how many choices to choose from. for efficient generation. improved
  // locality.
  size_t *choices_list;

  struct pattern_repeat repeat;
};

/// Segment. Represents something like `abc[def](g|h)`
struct pattern_segments {
  struct pattern_substring pos;

  // array of segment_items
  passgen_array_t items;

  // struct pattern_repeat repeat;
};

/// Group. Represents something like `abc[d0]|[def]g|(h|i)j`.
struct pattern_group {
  struct pattern_substring pos;

  // array of segments
  passgen_array_t segments;

  struct pattern_repeat repeat;
};

/// Special char.
struct pattern_special {
  struct pattern_substring pos;
  struct pattern_substring arg;
  enum pattern_special_kind kind;
  struct pattern_repeat length;
  struct pattern_repeat repeat;
  union {
    enum passgen_pronounceable_type pronounceable;
  } data;
};

/// Pattern segment: single segment (range, group or sequence).
struct pattern_segment {
  enum pattern_kind kind;

  union {
    struct pattern_ranges range;
    struct pattern_group group;
    struct pattern_char character;
    struct pattern_special special;
  } data;
};

/// Pattern: list of possible pattern segments as linked list.
struct pattern {
  passgen_mem_t *mem;
  struct pattern_group group;
  const char *pattern;
};

enum pattern_error {
  PATTERN_ERROR_ALLOC,
  PATTERN_ERROR_ILLEGAL,
  PATTERN_ERROR_DEPTH,
  PATTERN_ERROR_GROUP_CLOSE,
  PATTERN_ERROR_LAST,
};

struct pattern_result {
  bool ok;
  bool remove;
  struct pattern_substring pos;
  enum pattern_error kind;
};

struct pattern_env {
  bool find_complexity;
  double complexity;
  size_t pronounceable_limit;
  enum passgen_pronounceable_type pronounceable_type;
};

typedef struct pattern pattern_t;
typedef struct pattern_substring pattern_substring_t;
typedef struct pattern_result pattern_result_t;
typedef struct pattern_segments pattern_segments_t;
typedef struct pattern_segment pattern_segment_t;
typedef struct pattern_char pattern_char_t;
typedef struct pattern_special pattern_special_t;
typedef struct pattern_group pattern_group_t;
typedef struct pattern_ranges pattern_ranges_t;
typedef struct pattern_range pattern_range_t;
typedef struct pattern_env pattern_env_t;

typedef struct pattern_repeat pattern_repeat_t;

typedef int pattern_passgen_random_cb(void *data, int32_t codepoint);

void pattern_free(pattern_t *pattern);

pattern_result_t pattern_parse(
    pattern_t *out,
    const char *data,
    size_t depth,
    passgen_mem_t *mem);

size_t pattern_maxlen(pattern_t *pattern);
size_t pattern_minlen(pattern_t *pattern);

size_t pattern_passgen_random_fill(
    pattern_t *pattern,
    passgen_random_t *rand,
    pattern_env_t *env,
    char *buffer,
    size_t len);

int pattern_random(
    pattern_t *pattern,
    passgen_random_t *rand,
    pattern_env_t *env,
    void *data,
    pattern_passgen_random_cb *func);

size_t pattern_choices(pattern_t *pattern);

const char *pattern_error_to_str(enum pattern_error kind);
void pattern_error_show(pattern_result_t result, const char *format);

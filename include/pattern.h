#pragma once
#include <stdlib.h>
#include "random.h"

// TODO: remove PATTERN_CHAR, convert to use PATTERN_RANGE.

// What type of patterns are there?
typedef enum {
  // Range of possible chars.
  PATTERN_RANGE,
  // Single char.
  PATTERN_CHAR,
  // Group (subpattern).
  PATTERN_GROUP,
} pattern_kind;

struct pattern_range_t;
struct pattern_segment_t;
struct pattern_t;

// Range of possible characters. Linked list of [start, end] pairs.
struct pattern_range_t {
  char start;
  char end;
  struct pattern_range_t *next;
};

// Represents how many times a pattern must be repeated.
typedef struct {
  size_t min;
  size_t max;
} pattern_reps_t;

// Pattern segment: chain of patterns, as linked list.
struct pattern_segment_t {
  // What kind of segment is this?
  pattern_kind kind;
  // The segment itself, as union.
  union {
    struct pattern_range_t *range;
    struct pattern_t *group;
    const char *chr;
    void *raw;
  } data;
  // How many times this item is repeated.
  pattern_reps_t reps;
  // What comes after this item.
  struct pattern_segment_t *next;
};

// Pattern: list of possible pattern segments as linked list.
struct pattern_t {
  struct pattern_segment_t *item;
  struct pattern_t *next;
};

typedef struct pattern_t pattern_t;
typedef struct pattern_range_t pattern_range_t;
typedef struct pattern_segment_t pattern_segment_t;

pattern_t *pattern_parse(const char **string);
size_t pattern_maxlen(pattern_t *pattern);
size_t pattern_random_fill(pattern_t *pattern, random_t *rand, char *buffer, size_t len);
char *pattern_random(pattern_t *pattern, random_t *rand);
size_t pattern_segment_count(pattern_t *pattern);

pattern_range_t *pattern_range_new(char start, char end, pattern_range_t *next);
pattern_range_t *pattern_range_parse(const char **string);
void pattern_range_free(pattern_range_t *range);
char pattern_range_random(pattern_range_t *range, random_t *rand);

pattern_reps_t pattern_segment_parse_reps(const char **string);

pattern_segment_t *pattern_segment_new(pattern_kind kind, void *data, pattern_reps_t reps, pattern_segment_t *next);
pattern_segment_t *pattern_segment_parse(const char **string);
void pattern_segment_free(pattern_segment_t *pattern);

size_t pattern_segment_maxlen(pattern_segment_t *pattern);
size_t pattern_segment_random_fill(pattern_segment_t *pattern, random_t *rand, char *buffer, size_t len);
char *pattern_segment_random(pattern_segment_t *pattern, random_t *rand);

#pragma once
#include <stdlib.h>
#include "random.h"

typedef enum {
  PATTERN_RANGE,
  PATTERN_CHAR,
  PATTERN_GROUP,
} pattern_kind;

struct pattern_range_t;
struct pattern_range_t {
  char start;
  char end;
  struct pattern_range_t *next;
};

typedef struct pattern_range_t pattern_range_t;

typedef struct {
  size_t min;
  size_t max;
} pattern_reps_t;

struct pattern_segment_t;

struct pattern_segment_t {
  pattern_kind kind;
  void *data;
  pattern_reps_t reps;
  struct pattern_segment_t *next;
};

typedef struct pattern_segment_t pattern_segment_t;

pattern_range_t *pattern_range_new(char start, char end, pattern_range_t *next);
pattern_range_t *pattern_range_parse(const char **string);
void pattern_range_free(pattern_range_t *range);

pattern_reps_t pattern_segment_parse_reps(const char **string);

pattern_segment_t *pattern_segment_new(pattern_kind kind, void *data, pattern_reps_t reps, pattern_segment_t *next);
pattern_segment_t *pattern_segment_parse(const char **string);
void pattern_segment_free(pattern_segment_t *pattern);

size_t pattern_segment_maxlen(pattern_segment_t *pattern);
size_t pattern_segment_random_fill(pattern_segment_t *pattern, random_t *rand, char *buffer, size_t len);
char *pattern_segment_random(pattern_segment_t *pattern, random_t *rand);

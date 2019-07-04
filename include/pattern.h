#pragma once
#include <stdlib.h>

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

struct pattern_t;

struct pattern_t {
  pattern_kind kind;
  void *data;
  pattern_reps_t reps;
  struct pattern_t *next;
};

typedef struct pattern_t pattern_t;

pattern_range_t *pattern_range_new(char start, char end, pattern_range_t *next);
pattern_range_t *pattern_range_parse(const char **string);
void pattern_range_free(pattern_range_t *range);

pattern_reps_t pattern_parse_reps(const char **string);

pattern_t *pattern_new(pattern_kind kind, void *data, pattern_reps_t reps, pattern_t *next);
pattern_t *pattern_parse(const char **string);
void pattern_free(pattern_t *pattern);

#pragma once
#include <stdlib.h>

typedef enum {
  PATTERN_RANGE,
  PATTERN_CHAR
} pattern_kind;

struct pattern_range_t;
struct pattern_range_t {
  char start;
  char end;
  struct pattern_range_t *next;
};

typedef struct pattern_range_t pattern_range_t;

struct pattern_t;

struct pattern_t {
  pattern_kind kind;
  struct pattern_t *next;
};

typedef struct pattern_t pattern_t;

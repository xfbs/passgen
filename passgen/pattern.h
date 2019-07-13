/// @file pattern.h
/// @author Patrick M. Elsen
///
/// Methods for parsing a regex-subset used to generate custom data.

#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include "random.h"

// TODO: remove PATTERN_CHAR, convert to use PATTERN_RANGE.
// TODO: implement special chars \w (word), \s (space), etc.

/// What type of patterns are there?
typedef enum {
  /// Range of possible chars.
  PATTERN_RANGE,
  /// Single char.
  PATTERN_CHAR,
  /// Group (subpattern).
  PATTERN_GROUP,
} pattern_kind;

struct pattern_range_t;
struct pattern_segment_t;
struct pattern_t;

/// Range of possible characters. Linked list of [start, end] pairs.
struct pattern_range_t {
  /// Lowest possible character.
  char start;
  /// Highest possible character.
  char end;
  /// Next pair of possible characters.
  struct pattern_range_t *next;
};

/// Represents how many times a pattern must be repeated.
typedef struct {
  /// Minimum amount of times it must be repeated.
  size_t min;
  /// Maximum amount of times it must be repeated.
  size_t max;
} pattern_reps_t;

/// Pattern segment: chain of patterns, as linked list.
struct pattern_segment_t {
  /// What kind of segment is this?
  pattern_kind kind;
  /// The segment itself, as union.
  union {
    struct pattern_range_t *range;
    struct pattern_t *group;
    const char *chr;
    void *raw;
  } data;
  /// How many times this item is repeated.
  pattern_reps_t reps;
  //. What comes after this item.
  struct pattern_segment_t *next;
};

/// Pattern: list of possible pattern segments as linked list.
struct pattern_t {
  struct pattern_segment_t *item;
  struct pattern_t *next;
};

enum pattern_error_kind {
  PATTERN_ERROR_UNKNOWN,
  PATTERN_ERROR_ALLOC,
};

struct pattern_error_t {
  enum pattern_error_kind kind;
  size_t prev;
  size_t pos;
};

struct pattern_range_result_t {
  bool ok;
  union {
    struct pattern_range_t *range;
    struct pattern_error_t error;
  } data;
};

typedef struct pattern_t pattern_t;
typedef struct pattern_range_t pattern_range_t;
typedef struct pattern_segment_t pattern_segment_t;

typedef struct pattern_range_result_t pattern_range_result_t;
typedef struct pattern_error_t pattern_error_t;

/// Allocate a new pattern.
pattern_t *pattern_new(pattern_segment_t *segment, pattern_t *next);

/// Free a pattern created by pattern_new() or pattern_parse().
void pattern_free(pattern_t *pattern);

/// Parse a pattern.
pattern_t *pattern_parse(const char **string);

/// Determine how long the longest possible string that could be generated from
/// this pattern is.
size_t pattern_maxlen(pattern_t *pattern);

/// Use a pattern to fill a buffer with a string matching that pattern.
size_t pattern_random_fill(pattern_t *pattern, random_t *rand, char *buffer,
                           size_t len);

/// Generate a string matching the pattern @p pattern using @p as a randomness
/// source.
char *pattern_random(pattern_t *pattern, random_t *rand);

/// Find out how many pattern segments this pattern contains.
size_t pattern_segment_count(pattern_t *pattern);

/// Find out how many possible (not necessarily distinct) strings can be
/// generated from this pattern.
size_t pattern_choices(pattern_t *pattern);


/// Creates a new pattern error object.
pattern_error_t pattern_error(enum pattern_error_kind kind, size_t prev, size_t pos);

/// Format string to print out a pattern error object.
const char *pattern_error_fmtstr(pattern_error_t error);

pattern_range_t *pattern_range_new(char start, char end, pattern_range_t *next);
pattern_range_t *pattern_range_parse(const char **string);
void pattern_range_free(pattern_range_t *range);
char pattern_range_random(pattern_range_t *range, random_t *rand);
size_t pattern_range_choices(pattern_range_t *range);

pattern_reps_t pattern_segment_parse_reps(const char **string);

pattern_segment_t *pattern_segment_new(pattern_kind kind, void *data,
                                       pattern_reps_t reps,
                                       pattern_segment_t *next);
pattern_segment_t *pattern_segment_parse(const char **string);
void pattern_segment_free(pattern_segment_t *pattern);
size_t pattern_segment_choices(pattern_segment_t *segment);

size_t pattern_segment_maxlen(pattern_segment_t *pattern);
size_t pattern_segment_random_fill(pattern_segment_t *pattern, random_t *rand,
                                   char *buffer, size_t len);
char *pattern_segment_random(pattern_segment_t *pattern, random_t *rand);

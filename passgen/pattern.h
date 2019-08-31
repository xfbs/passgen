/// @file pattern.h
/// @author Patrick M. Elsen
///
/// Methods for parsing a regex-subset used to generate custom data.

#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include "random.h"
#include "unicode.h"
#include "token.h"
#include "memory.h"
#include "array.h"

enum pattern_kind {
  PATTERN_RANGE,
  PATTERN_CHAR,
  PATTERN_SPECIAL,
  PATTERN_GROUP,
};

struct pattern;
struct pattern_segment;
struct pattern_segment_item;
struct pattern_char;
struct pattern_special;
struct pattern_group;
struct pattern_range;
struct pattern_range_item;

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
struct pattern_range_item {
    struct pattern_substring pos;

    int32_t start;
    int32_t end;

    size_t multiplier;
};

/// Range. Represents something like `[a-z012]{3}`.
struct pattern_range {
    struct pattern_substring pos;

    // array of range_items
    passgen_array_t items;

    struct pattern_repeat repeat;
};

/// Segment. Represents something like `abc[def](g|h)`
struct pattern_segment {
    struct pattern_substring pos;

    // array of segment_items
    passgen_array_t items;

    struct pattern_repeat repeat;
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

    struct pattern_repeat length;
    struct pattern_repeat repeat;
};

/// Pattern segment: single segment (range, group or sequence).
struct pattern_segment_item {
    enum pattern_kind kind;

    union {
        struct pattern_range range;
        struct pattern_group group;
        struct pattern_char sequence;
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
};

struct pattern_result {
    bool ok;
    struct pattern_substring pos;
    enum pattern_error kind;
};

typedef struct pattern pattern_t;
typedef struct pattern_result pattern_result_t;
typedef struct pattern_segment pattern_segment_t;
typedef struct pattern_segment_item pattern_segment_item_t;
typedef struct pattern_char pattern_char_t;
typedef struct pattern_special pattern_special_t;
typedef struct pattern_group pattern_group_t;
typedef struct pattern_range pattern_range_t;
typedef struct pattern_range_item pattern_range_item_t;

void pattern_free(pattern_t *pattern);

pattern_result_t pattern_parse(pattern_t *out, const char *data, passgen_mem_t *mem);

size_t pattern_maxlen(pattern_t *pattern);
size_t pattern_minlen(pattern_t *pattern);

size_t pattern_random_fill(
    pattern_t *pattern,
    random_t *rand,
    char *buffer,
    size_t len);

size_t pattern_choices(pattern_t *pattern);

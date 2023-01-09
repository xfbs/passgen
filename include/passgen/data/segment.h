/// @file segment.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern segment data structure and methods.
#pragma once
#include <stddef.h>

#include "passgen/stack.h"

struct passgen_pattern_item;
struct passgen_pattern_set;
struct passgen_pattern_char;
struct passgen_pattern_group;
struct passgen_pattern_special;

/// Represents a single segment of a pattern. 
///
/// For example, in the group
/// `(word|tree)`, the two segments are `word` and `tree`. Segment are composed
/// of sequences of segment items, which can be anything (other groups, single
/// characters, ranges or possible characters).
typedef struct passgen_pattern_segment {
    // array of segment_items
    passgen_stack items;
} passgen_pattern_segment_t;

void passgen_pattern_segment_init(struct passgen_pattern_segment *segment);
void passgen_pattern_segment_free(struct passgen_pattern_segment *segment);

struct passgen_pattern_item *
passgen_pattern_segment_new_item(struct passgen_pattern_segment *segment);

struct passgen_chars *
passgen_pattern_segment_new_char(struct passgen_pattern_segment *segment);
struct passgen_pattern_set *
passgen_pattern_segment_new_set(struct passgen_pattern_segment *segment);
struct passgen_pattern_group *
passgen_pattern_segment_new_group(struct passgen_pattern_segment *segment);
struct passgen_pattern_special *
passgen_pattern_segment_new_special(struct passgen_pattern_segment *segment);

struct passgen_pattern_item *passgen_pattern_segment_get_item(
    struct passgen_pattern_segment *segment,
    size_t n);

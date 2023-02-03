/// @file segment.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern segment data structure (such as the `abc` in `(abc|def)`)
#pragma once
#include <stddef.h>

#include "passgen/container/stack.h"
#include "passgen/pattern/segment_item.h"

/// Pattern segment, such as `[a-z]{4}abc`
///
/// Pattern segments are parts of
/// [passgen_pattern_groups](#passgen_pattern_groups) and contain
/// [passgen_pattern_items](#passgen_pattern_item).
///
/// For example, in the group `(word|tree)`, the two segments are `word` and
/// `tree`. Segment are composed of sequences of segment items, which can be
/// anything (other groups, single characters, ranges or possible characters).
typedef struct passgen_pattern_segment {
    /// List of [passgen_pattern_items](#passgen_pattern_items).
    passgen_stack items;

    /// Multiplier for this segment
    size_t multiplier;
} passgen_pattern_segment;

/// Allocate new segment
///
/// @memberof passgen_pattern_segment
void passgen_pattern_segment_init(passgen_pattern_segment *segment);

/// Release segment memory
///
/// @memberof passgen_pattern_segment
void passgen_pattern_segment_free(passgen_pattern_segment *segment);

/// Appends new empty segment item onto this segment
///
/// @memberof passgen_pattern_segment
passgen_pattern_item *
passgen_pattern_segment_new_item(passgen_pattern_segment *segment);

/// Appends new @ref passgen_pattern_literal onto this segment.
///
/// @memberof passgen_pattern_segment
passgen_pattern_literal *
passgen_pattern_segment_new_char(passgen_pattern_segment *segment);

/// Appends new @ref passgen_pattern_set onto this segment.
///
/// @memberof passgen_pattern_segment
passgen_pattern_set *
passgen_pattern_segment_new_set(passgen_pattern_segment *segment);

/// Appends new @ref [passgen_pattern_group] onto this segment.
///
/// @memberof passgen_pattern_segment
passgen_pattern_group *
passgen_pattern_segment_new_group(passgen_pattern_segment *segment);

/// Appends new @ref passgen_pattern_special onto this segment.
///
/// @memberof passgen_pattern_segment
passgen_pattern_special *
passgen_pattern_segment_new_special(passgen_pattern_segment *segment);

/// Lookup the [n]th segment item of this segment.
///
/// @memberof passgen_pattern_segment
passgen_pattern_item *passgen_pattern_segment_get_item(
    passgen_pattern_segment *segment,
    size_t n);

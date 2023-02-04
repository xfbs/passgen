/// @file group.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern group (such as `(abc|def)`)
///
/// Data structure to hold pattern groups. A pattern group is a sequence like `(abc|def)`, where the
/// subsequences `abc` and `def` are two *segments* of the group. That is, when generating sequences,
/// passgen will pick either of these segments at random. The @ref passgen_pattern_group struct is how
/// these groups are represented in memory.
#pragma once
#include <stddef.h>

#include "passgen/container/stack.h"

typedef struct passgen_pattern_segment passgen_pattern_segment;

/// Pattern group, such as `(day|night)`.
///
/// Groups consist of [passgen_pattern_segments](#passgen_pattern_segment),
/// which are delimited by pipe characters (`|`). When generating a password
/// from the group, a segment is chosen at random.
///
/// [Segments][#passgen_pattern_segments] can have multipliers in front of
/// them, which changes the probability that they are chosen. For example:
/// `({5}abc|{1}def)`. In this example, the `abc` pattern is chosen at a 5:1
/// ratio. If no multipliers are specified, then every segment is chosen at the
/// same probability. Their documentation explains this in more detail. To make
/// generation work correctly, the group keeps track of the #multiplier_sum,
/// which is the sum of all of the multipliers of all segments. This needs to
/// be computed using the #passgen_pattern_group_finish method.
typedef struct {
    /// [Segments][#passgen_pattern_segment] that make up this group
    passgen_stack segments;

    /// Sum of segment multipliers
    size_t multiplier_sum;
} passgen_pattern_group;

/// Initialize a pattern group.
///
/// @memberof passgen_pattern_group
/// @param group Pattern group to initialize
void passgen_pattern_group_init(passgen_pattern_group *group);

/// Finishes a pattern group.
///
/// This will compute the #multiplier_sum from the segments inside the group.
/// Must be called after parsing the group is done to ensure accurate generation.
///
/// @memberof passgen_pattern_group
/// @param group Pattern group to finish
void passgen_pattern_group_finish(passgen_pattern_group *group);

/// Free a pattern group.
///
/// This will free all of the segments contained in the group. It will not free the
/// memory of the group struct itself.
///
/// @memberof passgen_pattern_group
/// @param group Pattern group to free
void passgen_pattern_group_free(passgen_pattern_group *group);

/// Appends a new segment to an existing #passgen_pattern_group.
///
/// @memberof passgen_pattern_group
/// @param group Group to append segment to
/// @returns Point to the newly appended segment
passgen_pattern_segment *
passgen_pattern_group_segment_append(passgen_pattern_group *group);

/// Get a specific segment from a @ref passgen_pattern_group by index.
///
/// @memberof passgen_pattern_group
/// @param group Group to lookup segment in
/// @param n Offset of segment to return
/// @returns Pointer to the segment or NULL if it does not exist
passgen_pattern_segment *
passgen_pattern_group_segment_get(passgen_pattern_group *group, size_t n);

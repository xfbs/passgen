/// @file group.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern group
///
/// Data structure to hold pattern groups. A pattern group is a sequence like `(abc|def)`, where the
/// subsequences `abc` and `def` are two *segments* of the group. That is, when generating sequences,
/// passgen will pick either of these segments at random. The @ref passgen_pattern_group struct is how
/// these groups are represented in memory.
#pragma once
#include <stddef.h>

#include "passgen/util/stack.h"
#include "passgen/pattern/repeat.h"

/// Represents a group, such as `(day|night)`.
typedef struct passgen_pattern_group {
    /// Segments that make up this group
    passgen_stack segments;
} passgen_pattern_group;

/// Initialize a @ref passgen_pattern_group.
void passgen_pattern_group_init(passgen_pattern_group *group);

/// Free a @ref passgen_pattern_group.
void passgen_pattern_group_free(passgen_pattern_group *group);

/// Add a new segment to an existing @ref passgen_pattern_group.
struct passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group *group);

/// Get a specific segment from a @ref passgen_pattern_group by index.
struct passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group *group, size_t n);

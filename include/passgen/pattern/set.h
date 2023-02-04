/// @file set.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern sets (such as the `[a-zA-Z0-9]``)
#pragma once
#include <stddef.h>

#include "passgen/container/stack.h"
#include "passgen/container/array.h"
#include "passgen/pattern/range.h"

/// Pattern set, such as `[a-zA-Z0-9]`.
///
/// A set represents a set of possible characters. It contains
/// [passgen_pattern_ranges](#passgen_pattern_range). Every character listed in
/// the set has an equal probability of occuring.
///
/// For example, `[a-z0-9]` is a set expression that can produce lowercase latin
/// characters or numbers.
typedef struct {
    /// Items in this set.
    /// This is a stack of #passgen_pattern_range items.
    passgen_stack items;

    /// Array of possible choices for every range in this set.
    /// This is a pointer to an array that has `items.len` entries.
    size_t *choices_list;
} passgen_pattern_set;

/// Initialize a new #passgen_pattern_set.
///
/// @memberof passgen_pattern_set
/// @param set Set to initialize
void passgen_pattern_set_init(passgen_pattern_set *set);

/// Free this #passgen_pattern_set.
///
/// This will free all of the ranges contained within the set.
///
/// @memberof passgen_pattern_set
/// @param set Set to free
void passgen_pattern_set_free(passgen_pattern_set *set);

/// Append a new #passgen_pattern_range onto the pattern set.
///
/// @memberof passgen_pattern_set
/// @param set Set to append onto
/// @returns Pointer to the appended range
passgen_pattern_range *
passgen_pattern_set_range_append(passgen_pattern_set *set);

/// Get the nth #passgen_pattern_range from this set.
///
/// @memberof passgen_pattern_set
/// @param set Set to lookup range in
/// @param n Offset of range to get
/// @returns Pointer to the range if exists, else NULL.
passgen_pattern_range *
passgen_pattern_set_range_get(passgen_pattern_set *set, size_t n);

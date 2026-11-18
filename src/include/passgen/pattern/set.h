/// @file set.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern sets (such as the `[a-zA-Z0-9]``)
#pragma once
#include <stddef.h>

#include "passgen/container/stack.h"
#include "passgen/pattern/range.h"

/// Pattern set, such as `[a-zA-Z0-9]`.
///
/// A set represents a set of possible characters. It contains
/// [passgen_pattern_ranges](#passgen_pattern_range). Every time a character
/// is listed in the set counts as one choice, so in `[aab]`, `a` is twice as
/// likely as `b`.
///
/// For example, `[a-z0-9]` is a set expression that can produce lowercase latin
/// characters or numbers.
///
/// Once the set is complete (see @ref passgen_pattern_set_finish), its ranges
/// are sorted, do not overlap, and have the number of times their characters
/// were listed as their multiplier. This way, sets which list the same
/// characters (such as `[ba]`, `[ab]` and `[a-b]`) are the same.
typedef struct {
    /// Items in this set.
    /// This is a stack of #passgen_pattern_range items.
    passgen_stack items;

    /// The number of choices up to and including every range in this set,
    /// taking the multipliers into account. This is a pointer to an array that
    /// has `items.len` entries, or NULL if the set is not complete.
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

/// Completes a #passgen_pattern_set once all of its ranges were appended.
///
/// Sorts the ranges, splits them so that they do not overlap, and sets the
/// multiplier of every range to the number of ranges which contained its
/// characters. Adjacent ranges with the same multiplier are merged. Then fills
/// in the list of choices.
///
/// @memberof passgen_pattern_set
/// @param set Set to complete, which must have at least one range
/// @returns Zero on success, @ref PASSGEN_ERROR_OUT_OF_MEMORY if memory ran
///     out, or @ref PASSGEN_ERROR_SET_RANGE if the number of choices does not
///     fit into a `size_t`. On failure, the set is unchanged.
int passgen_pattern_set_finish(passgen_pattern_set *set);

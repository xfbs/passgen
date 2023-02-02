/// @file set.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern sets (such as the `[a-zA-Z0-9]``)
#pragma once
#include <stddef.h>

#include "passgen/container/stack.h"
#include "passgen/container/array.h"
#include "passgen/pattern/repeat.h"

struct passgen_pattern_range;

/// A set represents a set of possible characters.
///
/// For example, `[a-z0-9]` is a set expression that can produce lowercase latin
/// characters of numbers. A set is an array of @ref passgen_pattern_range.
typedef struct passgen_pattern_set {
    // array of range_items
    passgen_stack items;

    // Array of possible choices for every range in this set.
    size_t *choices_list;
} passgen_pattern_set;

/// Initialize a new [passgen_pattern_set].
void passgen_pattern_set_init(passgen_pattern_set *set);

/// Free this [passgen_pattern_set].
void passgen_pattern_set_free(passgen_pattern_set *set);

/// Append a new @ref passgen_pattern_range onto @ref set.
struct passgen_pattern_range *
passgen_pattern_set_new_range(passgen_pattern_set *set);

/// Get the [n]th [passgen_pattern_range] from this [set].
struct passgen_pattern_range *
passgen_pattern_set_get_range(passgen_pattern_set *set, size_t n);

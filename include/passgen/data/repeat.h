/// @file repeat.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern repetition data structure
///
/// Groups, characters and ranges can be repeated, by following them with braces with either a fixed amount
/// of repetitions or a min, max range. The @ref passgen_pattern_repeat struct is how these repetition counts
/// are stored in memory.
#pragma once
#include <stddef.h>
#include "passgen/debug.h"

/// Represents a repetition of the preceding item.
///
/// For example, `a{4}` evaluates to `aaaa`. But specifying `a{1,4}` results
/// in between 1 to 4 repetition of `a`. The range is inclusive.
typedef struct passgen_pattern_repeat {
    /// Minimum number of repetitions
    size_t min;
    /// Maximum number of repetitions
    size_t max;
} passgen_repeat_t;

void passgen_pattern_repeat_debug(passgen_repeat_t *repeat, passgen_debug_t *debug);

/// @file repeat.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern repetition data structure (such as the `{4}` in `[a-z]{4}`)
///
/// Groups, characters and ranges can be repeated, by following them with braces with either a fixed amount
/// of repetitions or a min, max range. The @ref passgen_pattern_repeat struct is how these repetition counts
/// are stored in memory.
#pragma once
#include <stddef.h>

/// Pattern repetition, such as `{5}` or `{3,5}`.
///
/// Represents a repetition of the preceding item.
///
/// For example, `a{4}` evaluates to `aaaa`. But specifying `a{1,4}` results
/// in between 1 to 4 repetition of `a`. The range is inclusive.
typedef struct passgen_pattern_repeat {
    /// Minimum number of repetitions
    size_t min;
    /// Maximum number of repetitions
    size_t max;
} passgen_pattern_repeat;

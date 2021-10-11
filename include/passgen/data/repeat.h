#pragma once
#include <stddef.h>
#include "passgen/debug.h"

/// Represents a repetition of the preceding item.
///
/// For example, `a{4}` evaluates to `aaaa`. But specifying `a{1,4}` results
/// in between 1 to 4 repetition of `a`. The range is inclusive.
typedef struct passgen_pattern_repeat {
    size_t min;
    size_t max;
} passgen_repeat_t;

void passgen_pattern_repeat_debug(passgen_repeat_t *repeat, passgen_debug_t *debug);

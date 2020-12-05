#pragma once
#include "passgen/data/substring.h"
#include <stddef.h>
#include <stdint.h>

struct passgen_pattern_range {
    struct passgen_pattern_substring pos;

    int32_t start;
    int32_t end;

    size_t multiplier;
};

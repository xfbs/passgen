#pragma once
#include "substring.h"
#include "passgen/array.h"
#include "repeat.h"

struct passgen_pattern_group {
    struct passgen_pattern_substring pos;

    // array of segments
    passgen_array_t segments;

    struct passgen_pattern_repeat repeat;
};

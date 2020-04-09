#pragma once
#include <stdint.h>
#include "substring.h"
#include "repeat.h"

struct passgen_pattern_char {
    struct passgen_pattern_substring pos;
    int32_t codepoint;
    struct passgen_pattern_repeat repeat;
};

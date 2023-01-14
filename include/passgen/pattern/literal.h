#pragma once
#include "passgen/pattern/repeat.h"
#include <stdint.h>

// stores a bunch of character - up to seven. why seven? this is stored in an
// union inside passgen_pattern_item, and there's enough space in there to
// store seven codepoints and a count variable, so why not.
typedef struct passgen_pattern_literal {
    uint8_t count;
    uint8_t tainted;
    int32_t codepoints[7];
} passgen_pattern_literal;

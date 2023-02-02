/// @file literal.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern literal (such as `abc`)
#pragma once
#include "passgen/pattern/repeat.h"
#include <stdint.h>

// stores a bunch of character - up to seven. why seven? this is stored in an
// union inside passgen_pattern_item, and there's enough space in there to
// store seven codepoints and a count variable, so why not.
typedef struct passgen_pattern_literal {
    /// How many characters are stored here
    uint8_t count;
    /// If this literal is tainted, new characters must go into a new, empty @ref passgen_pattern_literal.
    uint8_t tainted;
    /// Space for seven characters to be encoded.
    int32_t codepoints[7];
} passgen_pattern_literal;

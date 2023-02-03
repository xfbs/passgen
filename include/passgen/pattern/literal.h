/// @file literal.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern literal (such as `abc`)
#pragma once
#include "passgen/pattern/repeat.h"
#include <stdint.h>

/// Literal pattern (such as `abc`).
///
/// Literal patterns consist of codepoints that are generated verbatim.
/// There is enough space in the struct to store up to seven codepoints.
///
/// Literal patterns also store a tainted flag, this is set to true if
/// the last codepoint has modifiers on it.
typedef struct {
    /// How many characters are stored here
    uint8_t count;
    /// If this literal is tainted, new characters must go into a new, empty @ref passgen_pattern_literal.
    uint8_t tainted;
    /// Space for seven characters to be encoded.
    int32_t codepoints[7];
} passgen_pattern_literal;

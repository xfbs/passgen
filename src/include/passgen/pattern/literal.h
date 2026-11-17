/// @file literal.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern literal (such as `abc`)
#pragma once
#include "passgen/pattern/repeat.h"
#include <stdint.h>

/// Pattern literal, such as `abc`.
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

/// Initialize a pattern literal.
/// @param literal Pattern literal to initialize
/// @memberof passgen_pattern_literal
void passgen_pattern_literal_init(passgen_pattern_literal *literal);

/// Taint a pattern literal.
/// Doing so prevents further appending to the literal.
/// @param literal Pattern literal to taint
void passgen_pattern_literal_taint(passgen_pattern_literal *literal);

/// Append codepoint to pattern literal.
/// @param literal Literal to append codepoint to
/// @param codepoint Codepoint to append to literal
/// @returns 0 on success, nonzero on failure (maximum capacity or tainted)
int passgen_pattern_literal_append(passgen_pattern_literal *literal, uint32_t codepoint);

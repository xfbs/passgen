/// @file range.h
/// @author Patrick M. Elsen
/// @brief Pattern range data structure.
///
/// A pattern range is something like `a-z`, which matches all characters in the range from `a` to `z`.
/// The @ref passgen_pattern_range struct is how these ranges are stored in memory.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "passgen/debug.h"
#include "passgen/export.h"

/// Represents a single range of characters, such as `a-z`.
///
///The range has a start and an end character, both are inclusive.
typedef struct passgen_pattern_range {
    int32_t start;
    int32_t end;

    size_t multiplier;
} passgen_pattern_range_t;

void passgen_pattern_range_debug(passgen_pattern_range_t *range, passgen_debug_t *debug);

int passgen_pattern_range_export(passgen_pattern_range_t *range, void *data, passgen_export_cb *fn);

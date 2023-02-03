/// @file range.h
/// @author Patrick M. Elsen
/// @brief Pattern range data structure (such as `a-z` in `[a-zA-Z0-9]`)
///
/// A pattern range is something like `a-z`, which matches all characters in
/// the range from `a` to `z`.  The @ref passgen_pattern_range struct is how
/// these ranges are stored in memory.
#pragma once
#include <stddef.h>
#include <stdint.h>

/// Range of characters, such as `a-z`.
///
/// The range has a start and an end character, both are inclusive. The end
/// character must be equal to or larger than the start character. If they are
/// the same, the range matches only the single character.
///
/// [Pattern sets](#passgen_pattern_set) consist of character ranges.
typedef struct {
    /// Starting character in the range
    uint32_t start;
    /// Final character in the end.
    uint32_t end;

    size_t multiplier;
} passgen_pattern_range;

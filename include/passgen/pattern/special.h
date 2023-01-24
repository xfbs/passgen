#pragma once
#include <stdint.h>

#include "passgen/pattern/repeat.h"
#include "passgen/pattern/special_kind.h"

/// Passgen pattern special
///
/// Represents a special pattern, such as `\w{name}` (to generate a random
/// word from a word list), `\m{name}` (to generate a random word from a
/// markov chain) or `\p{name}` (to reference a preset pattern).
typedef struct passgen_pattern_special {
    /// Kind of special pattern this is.
    enum passgen_pattern_special_kind kind;
    /// Current length of the parameters string.
    int32_t parameters_len;
    /// Capacity of the parameters string.
    int32_t parameters_cap;
    /// Parameters string, UTF-8 encoded.
    char *parameters;
} passgen_pattern_special;

/// Initialise a pattern special with the given char.
///
/// Char can be 'p' for pronounceable, 'w' for word list or 'p' for preset.
void passgen_pattern_special_init_char(
    passgen_pattern_special *special,
    uint32_t type);

/// Free a passgen_pattern_special instance.
void passgen_pattern_special_free(passgen_pattern_special *special);

/// Add a parameter codepoint to a passgen_pattern_special.
void passgen_pattern_special_add_parameter_cp(
    passgen_pattern_special *special,
    int32_t codepoint);

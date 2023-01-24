#pragma once
#include <stdint.h>

#include "passgen/pattern/repeat.h"
#include "passgen/pattern/special_kind.h"

typedef struct passgen_pattern_special {
    enum passgen_pattern_special_kind kind;
    int32_t parameters_len;
    int32_t parameters_cap;
    int32_t *parameters;
} passgen_pattern_special;

/// Initialise a pattern special with the given char.
///
/// Char can be 'p' for pronounceable or 'w' for word list.
void passgen_pattern_special_init_char(
    struct passgen_pattern_special *special,
    uint32_t type);

void passgen_pattern_special_free(struct passgen_pattern_special *special);

void passgen_pattern_special_add_parameter_cp(
    struct passgen_pattern_special *special,
    int32_t codepoint);

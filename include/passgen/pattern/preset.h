/// @file preset.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern presets
#pragma once
#include "passgen/pattern/pattern.h"
#include <stdbool.h>

/// Pattern preset.
typedef struct passgen_preset {
    /// Preset definition
    const char *value;
    /// Is preset parsed already?
    bool parsed;
    /// Parsed pattern of preset
    passgen_pattern pattern;
} passgen_preset;

/// Initialize a preset
void passgen_preset_init(passgen_preset *preset, const char *value);

/// Parse this preset, if it has not been already
int passgen_preset_parse(passgen_preset *preset);

/// Free this preset
void passgen_preset_free(passgen_preset *preset);

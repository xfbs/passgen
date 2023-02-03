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
///
/// @memberof passgen_preset
/// @param preset Preset to initialize
/// @param value Value of preset
void passgen_preset_init(passgen_preset *preset, const char *value);

/// Parse this preset, if it has not been already
///
/// @memberof passgen_preset
/// @param preset Preset to parse
/// @returns 0 on success, 1 on failure
int passgen_preset_parse(passgen_preset *preset);

/// Free this preset
///
/// @memberof passgen_preset
/// @param preset Preset to free
void passgen_preset_free(passgen_preset *preset);

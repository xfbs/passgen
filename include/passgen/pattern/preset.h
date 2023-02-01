/// @file preset.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Passgen pattern presets
#pragma once
#include "passgen/pattern/pattern.h"

/// Pattern preset.
typedef struct passgen_preset {
    const char *name;
    const char *preset;
    bool parsed;
    passgen_pattern pattern;
} passgen_preset;


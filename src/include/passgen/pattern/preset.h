/// @file preset.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern presets
#pragma once
#include "passgen/pattern/pattern.h"
#include <stdbool.h>
#include <stddef.h>

/// Pattern preset, which is a named pattern. Patterns use them as
/// `\p{name}`, and they can use other presets in turn.
///
/// They belong to an environment (see @ref passgen_env_preset_add), and are
/// parsed when a pattern which uses them is resolved for the first time.
typedef struct passgen_preset {
    /// The pattern of the preset, as it was defined. Owned by the preset.
    char *value;
    /// Whether @ref pattern was parsed from @ref value.
    bool parsed;
    /// Whether it is being resolved, so that a preset which uses itself
    /// (through others) is noticed.
    bool resolving;
    /// How deeply groups are nested in it, counting the preset itself as one
    /// level, and through the presets it uses. Only valid once it is
    /// resolved.
    size_t depth;
    /// Parsed pattern of the preset. It is resolved when its environment is
    /// the one of the preset (see @ref passgen_pattern::env).
    passgen_pattern pattern;
} passgen_preset;

/// Initialize a preset, which takes over the value.
///
/// @memberof passgen_preset
/// @param preset Preset to initialize
/// @param value Pattern of the preset, allocated with @ref
///   passgen_alloc_string
void passgen_preset_init(passgen_preset *preset, char *value);

/// Parse this preset, if it has not been already.
///
/// @memberof passgen_preset
/// @param preset Preset to parse
/// @param error Set to what went wrong if it cannot be parsed, with the
///   position in the pattern of the preset. May be NULL.
/// @returns @ref PASSGEN_OK on success
passgen_status
passgen_preset_parse(passgen_preset *preset, passgen_error *error);

/// Free what this preset holds, but not the preset itself.
///
/// @memberof passgen_preset
/// @param preset Preset to free
void passgen_preset_free(passgen_preset *preset);

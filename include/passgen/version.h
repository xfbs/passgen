/// @file version.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Version information for libpassgen.
#pragma once
#include <stdbool.h>
#include <stdint.h>

/// Version information for libpassgen.
typedef struct passgen_version {
    /// Major version
    uint8_t major;
    /// Minor version
    uint8_t minor;
    /// Patch version
    uint8_t patch;
    /// Build information
    const char *build;
    /// Prerelease information
    const char *prerelease;
} passgen_version;

/// Returns the version of libpassgen.
///
/// @related passgen_version
passgen_version passgen_version_get();

/// Test if the version of passgen is at least as big as the one passed.
///
/// @related passgen_version
/// @param version Minimum version that is expected
/// @returns `true` if the library is compatible
bool passgen_version_compatible(passgen_version version);

/// Checks if this is a debug build.
bool passgen_is_debug(void);

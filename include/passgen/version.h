/// @file version.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Version information for libpassgen.
#pragma once
#include <passgen/export.h>
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
    /// Build information, such as the commit it was built from. This is an
    /// empty string if there is none.
    const char *build;
    /// Prerelease information, such as `rc1`, or NULL for a release. None of
    /// the versions so far had any.
    const char *prerelease;
} passgen_version;

/// Returns the version of the library which is used, which may be another
/// one than that of the headers (see `PASSGEN_VERSION_MAJOR` and friends in
/// `passgen/config.h`).
///
/// @relatesalso passgen_version
PASSGEN_EXPORT passgen_version passgen_version_get(void);

/// Test if the version of passgen is compatible with the one passed, which
/// is typically the one that the caller was built against (see
/// `PASSGEN_VERSION_MAJOR` and friends in `passgen/config.h`).
///
/// This follows semantic versioning: the major version has to be the same,
/// and the minor and patch version must not be smaller. Before version 1.0.0,
/// the minor version has to be the same as well.
///
/// @relatesalso passgen_version
/// @param version Minimum version that is expected
/// @returns `true` if the library is compatible
PASSGEN_EXPORT bool passgen_version_compatible(passgen_version version);

/// Whether the library is a debug build. Those check more, which makes them
/// slower, and stop with a message when something which should not be
/// possible happens.
PASSGEN_EXPORT bool passgen_is_debug(void);

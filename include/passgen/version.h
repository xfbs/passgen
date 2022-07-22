/// @file version.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Version information for libpassgen.
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct passgen_version {
    uint8_t maj;
    uint8_t min;
    uint8_t rev;
} passgen_version;

/// Returns the version of passgen.
passgen_version passgen_version_get();

/// Returns a pointer to a string representing the current version.
const char *passgen_version_str();

/// Test if the version of passgen is at least as big as the one passed.
bool passgen_version_min(passgen_version version);

/// Checks if this is a debug build.
bool passgen_is_debug(void);

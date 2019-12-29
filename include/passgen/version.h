#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct passgen_version_t {
  uint8_t maj;
  uint8_t min;
  uint8_t rev;
} passgen_version_t;

/// Returns the version of passgen.
passgen_version_t passgen_version();

/// Returns a pointer to a string representing the current version.
const char *passgen_version_str();

/// Test if the version of passgen is at least as big as the one passed.
bool passgen_version_min(passgen_version_t version);

/// Checks if this is a debug build.
bool passgen_is_debug(void);

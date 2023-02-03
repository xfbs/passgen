/// @file pattern.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Parsed pattern
#pragma once

#include "passgen/pattern/group.h"

/// Represents an entire patten.
///
/// The root element of a pattern is a group, but with the difference that
/// it does not need to be enclosed in parentheses.
typedef struct passgen_pattern {
    /// Items in this pattern.
    passgen_pattern_group group;
} passgen_pattern;

/// Initialize this @ref pattern.
void passgen_pattern_init(passgen_pattern *pattern);

/// Deallocate this @ref pattern.
void passgen_pattern_free(passgen_pattern *pattern);

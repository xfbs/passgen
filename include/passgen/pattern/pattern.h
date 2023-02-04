/// @file pattern.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern structures and methods
#pragma once

#include "passgen/pattern/group.h"

/// Pattern, such as `abc(def|ghi)[a-z]{5}`.
///
/// The root element of a pattern is a group, but it does not need to be
/// enclosed in parantheses.
typedef struct {
    /// Items in this pattern.
    passgen_pattern_group group;
} passgen_pattern;

/// Initialize this pattern.
///
/// @memberof passgen_pattern
/// @param pattern Pattern to initialize
void passgen_pattern_init(passgen_pattern *pattern);

/// Deallocate this pattern.
///
/// @memberof passgen_pattern
/// @param pattern Pattern to free
void passgen_pattern_free(passgen_pattern *pattern);

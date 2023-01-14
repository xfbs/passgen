#pragma once

#include "passgen/pattern/group.h"

/// Represents an entire patten.
///
/// The root element of a pattern is a group, but with the difference that
/// it does not need to be enclosed in parentheses.
typedef struct passgen_pattern {
    struct passgen_pattern_group group;
} passgen_pattern;

void passgen_pattern_init(passgen_pattern *pattern);
void passgen_pattern_free(passgen_pattern *pattern);

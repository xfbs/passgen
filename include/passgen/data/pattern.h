#pragma once

#include "passgen/data/group.h"
#include "passgen/export.h"

/// Represents an entire patten.
///
/// The root element of a pattern is a group, but with the difference that
/// it does not need to be enclosed in parentheses.
typedef struct passgen_pattern {
    struct passgen_pattern_group group;
} passgen_pattern_t;

void passgen_pattern_init(passgen_pattern_t *pattern);
void passgen_pattern_free(passgen_pattern_t *pattern);

int passgen_pattern_export(passgen_pattern_t *pattern, void *data, passgen_export_cb *fn);

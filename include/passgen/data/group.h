#pragma once
#include <stddef.h>

#include "passgen/container/stack/segment.h"
#include "passgen/data/repeat.h"
#include "passgen/debug.h"
#include "passgen/export.h"

/// Represents a group, such as `(day|night)`.
typedef struct passgen_pattern_group {
    /// Segments that make up this group
    struct passgen_pattern_segment_stack segments;
} passgen_pattern_group_t;

void passgen_pattern_group_init(passgen_pattern_group_t *group);
void passgen_pattern_group_free(passgen_pattern_group_t *group);

struct passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group_t *group);

struct passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group_t *group, size_t n);

void passgen_pattern_group_debug(passgen_pattern_group_t *group, passgen_debug_t *debug);

int passgen_group_export(passgen_pattern_group_t *pattern, void *data, passgen_export_cb *fn);

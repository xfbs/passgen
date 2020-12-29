#pragma once
#include <stddef.h>

#include "passgen/container/stack/segment.h"
#include "passgen/data/repeat.h"
#include "passgen/data/substring.h"
#include "passgen/debug.h"

typedef struct passgen_pattern_group {
    // array of segments
    struct passgen_pattern_segment_stack segments;
} passgen_pattern_group_t;

void passgen_pattern_group_init(passgen_pattern_group_t *group);
void passgen_pattern_group_free(passgen_pattern_group_t *group);

struct passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group_t *group);

struct passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group_t *group, size_t n);

void passgen_pattern_group_debug(passgen_pattern_group_t *group, passgen_debug_t *debug);

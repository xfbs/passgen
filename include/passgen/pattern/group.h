#pragma once
#include "substring.h"
#include "passgen/array.h"
#include "passgen/pattern/segments.h"
#include "repeat.h"

typedef struct passgen_pattern_group {
    struct passgen_pattern_substring pos;

    // array of segments
    passgen_array_t segments;

    struct passgen_pattern_repeat repeat;
} passgen_pattern_group_t;

void passgen_pattern_group_init(passgen_pattern_group_t *group);

struct passgen_pattern_segments *passgen_pattern_group_add(passgen_pattern_group_t *group);

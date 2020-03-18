#pragma once

#include "ranges.h"
#include "group.h"
#include "char.h"
#include "special.h"
#include "kind.h"

typedef struct passgen_pattern_segment {
    enum passgen_pattern_kind kind;

    union {
        struct passgen_pattern_ranges range;
        struct passgen_pattern_group group;
        struct passgen_pattern_char character;
        struct passgen_pattern_special special;
    } data;
} passgen_pattern_segment_t;

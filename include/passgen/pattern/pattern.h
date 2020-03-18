#pragma once

#include "group.h"
#include "passgen/memory.h"

typedef struct passgen_pattern {
    passgen_mem_t *mem;
    struct passgen_pattern_group group;
    const char *pattern;
} passgen_pattern_t;

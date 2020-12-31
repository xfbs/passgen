#pragma once
#include <stddef.h>
#include "passgen/debug.h"

typedef struct passgen_pattern_repeat {
    size_t min;
    size_t max;
} passgen_repeat_t;

void passgen_pattern_repeat_debug(passgen_repeat_t *repeat, passgen_debug_t *debug);

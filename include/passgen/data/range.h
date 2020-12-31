#pragma once
#include "passgen/debug.h"
#include "passgen/export.h"
#include <stddef.h>
#include <stdint.h>

typedef struct passgen_pattern_range {
    int32_t start;
    int32_t end;

    size_t multiplier;
} passgen_pattern_range_t;

void passgen_pattern_range_debug(passgen_pattern_range_t *range, passgen_debug_t *debug);

int passgen_pattern_range_export(passgen_pattern_range_t *range, void *data, passgen_export_cb *fn);

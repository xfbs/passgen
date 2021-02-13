#pragma once

#include "passgen/data/group.h"
#include "passgen/debug.h"
#include "passgen/export.h"

typedef struct passgen_pattern {
    struct passgen_pattern_group group;
} passgen_pattern_t;

void passgen_pattern_init(passgen_pattern_t *pattern);
void passgen_pattern_free(passgen_pattern_t *pattern);

void passgen_pattern_debug(passgen_pattern_t *pattern, passgen_debug_t *debug);

int passgen_pattern_export(passgen_pattern_t *pattern, void *data, passgen_export_cb *fn);

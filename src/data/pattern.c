#include "passgen/data/pattern.h"

#include <stddef.h>

#include "passgen/data/group.h"

void passgen_pattern_init(passgen_pattern_t *pattern) {
    passgen_pattern_group_init(&pattern->group);
}

void passgen_pattern_free(passgen_pattern_t *pattern) {
    passgen_pattern_group_free(&pattern->group);
}

int passgen_pattern_export(
    passgen_pattern_t *pattern,
    void *data,
    passgen_export_cb *export) {
    return passgen_group_export(&pattern->group, data, export);
}

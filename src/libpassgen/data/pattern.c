#include "passgen/data/pattern.h"

#include <stddef.h>

#include "passgen/data/group.h"

void passgen_pattern_init(passgen_pattern_t *pattern) {
    passgen_pattern_group_init(&pattern->group);
    pattern->mem = NULL;
}

void passgen_pattern_free(passgen_pattern_t *pattern) {
    passgen_pattern_group_free(&pattern->group);
}

void passgen_pattern_debug(passgen_pattern_t *pattern, passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_pattern");

    debug->member_start(debug->data, "mem");
    debug->value_pointer(debug->data, pattern->mem);
    debug->member_end(debug->data, "mem");

    debug->member_start(debug->data, "group");
    passgen_pattern_group_debug(&pattern->group, debug);
    debug->member_end(debug->data, "group");

    debug->struct_end(debug->data, "passgen_pattern");
}

int passgen_pattern_export(
    passgen_pattern_t *pattern,
    void *data,
    passgen_export_cb *export) {
    return passgen_group_export(&pattern->group, data, export);
}

#include "passgen/config.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/pattern.h"

void passgen_pattern_init(passgen_pattern *pattern) {
    passgen_pattern_group_init(&pattern->group);
}

void passgen_pattern_free(passgen_pattern *pattern) {
    passgen_pattern_group_free(&pattern->group);
    PASSGEN_CLEAR(pattern);
}

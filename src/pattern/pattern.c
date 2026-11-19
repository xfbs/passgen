#include "passgen/pattern/pattern.h"
#include "passgen/config.h"
#include "passgen/pattern/group.h"
#include "passgen/util/clear.h"
#include "passgen/util/alloc.h"

void passgen_pattern_init(passgen_pattern *pattern) {
    passgen_pattern_group_init(&pattern->group);
    pattern->env = NULL;
    pattern->env_version = 0;
}

void passgen_pattern_deinit(passgen_pattern *pattern) {
    passgen_pattern_group_free(&pattern->group);
    PASSGEN_CLEAR(pattern);
}

void passgen_pattern_free(passgen_pattern *pattern) {
    if(!pattern) {
        return;
    }

    passgen_pattern_deinit(pattern);
    passgen_free(pattern, sizeof(*pattern));
}

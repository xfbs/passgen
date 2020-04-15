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

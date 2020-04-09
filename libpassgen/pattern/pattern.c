#include "passgen/pattern/pattern.h"
#include "passgen/pattern/group.h"

void passgen_pattern_init(passgen_pattern_t *pattern) {
  passgen_pattern_group_init(&pattern->group);
  pattern->mem = NULL;
  pattern->pattern = NULL;
}

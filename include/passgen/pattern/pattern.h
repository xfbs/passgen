#pragma once

#include "group.h"
#include "passgen/memory.h"

typedef struct passgen_pattern {
  passgen_mem_t *mem;
  struct passgen_pattern_group group;
} passgen_pattern_t;

void passgen_pattern_init(passgen_pattern_t *pattern);
void passgen_pattern_free(passgen_pattern_t *pattern);

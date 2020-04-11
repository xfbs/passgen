#pragma once
#include "passgen/container/stack.h"
#include "passgen/pattern/substring.h"
#include <stddef.h>
#include <stdint.h>

struct passgen_pattern_range {
  struct passgen_pattern_substring pos;

  int32_t start;
  int32_t end;

  size_t multiplier;
};

PASSGEN_STACK_DECLARE(struct passgen_pattern_range, passgen_pattern_range);

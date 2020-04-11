#pragma once
#include "passgen/data/repeat.h"
#include "passgen/pattern/substring.h"
#include <stdint.h>

struct passgen_pattern_char {
  struct passgen_pattern_substring pos;
  int32_t codepoint;
  struct passgen_pattern_repeat repeat;
};

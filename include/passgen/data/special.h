#pragma once
#include <stdint.h>

#include "passgen/data/repeat.h"
#include "passgen/data/special_kind.h"
#include "passgen/data/substring.h"

struct passgen_pattern_special {
  struct passgen_pattern_substring pos;
  struct passgen_pattern_substring arg;
  enum passgen_pattern_special_kind kind;
  struct passgen_pattern_repeat length;
  struct passgen_pattern_repeat repeat;
  /*
  union {
      enum passgen_pronounceable_type pronounceable;
  } data;
  */
};

void passgen_pattern_special_init_char(
    struct passgen_pattern_special *special,
    uint32_t type);

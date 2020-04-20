#pragma once
#include <stdint.h>

#include "passgen/data/repeat.h"
#include "passgen/data/special_kind.h"
#include "passgen/data/substring.h"

struct passgen_pattern_special {
  enum passgen_pattern_special_kind kind;
  struct passgen_pattern_repeat length;
  /*
  union {
      enum passgen_pronounceable_type pronounceable;
  } data;
  */
};

void passgen_pattern_special_init_char(
    struct passgen_pattern_special *special,
    uint32_t type);

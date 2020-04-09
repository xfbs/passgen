#pragma once

#include "passgen/pattern/char.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/kind.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"

typedef struct passgen_pattern_segment {
  enum passgen_pattern_kind kind;

  union {
    struct passgen_pattern_set set;
    struct passgen_pattern_group group;
    struct passgen_pattern_char character;
    struct passgen_pattern_special special;
  } data;
} passgen_pattern_segment_t;

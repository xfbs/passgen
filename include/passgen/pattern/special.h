#pragma once
#include "repeat.h"
#include "special_kind.h"
#include "substring.h"

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

#pragma once
#include "passgen/pronounceable.h"

struct passgen_env {
  bool find_complexity;
  double complexity;
  size_t pronounceable_limit;
  enum passgen_pronounceable_type pronounceable_type;
};


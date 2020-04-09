#pragma once
#include "passgen/array.h"
#include "substring.h"

typedef struct passgen_pattern_segments {
  struct passgen_pattern_substring pos;

  // array of segment_items
  passgen_array_t items;

  // struct pattern_repeat repeat;
} passgen_pattern_segments_t;

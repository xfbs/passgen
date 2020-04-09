#pragma once
#include "passgen/array.h"
#include "substring.h"

struct passgen_pattern_segment;

typedef struct passgen_pattern_segments {
  struct passgen_pattern_substring pos;

  // array of segment_items
  passgen_array_t items;

  // struct pattern_repeat repeat;
} passgen_pattern_segments_t;

void passgen_pattern_segments_init(struct passgen_pattern_segments *segments);

struct passgen_pattern_segment *passgen_pattern_segments_new_segment(struct passgen_pattern_segments *segments);

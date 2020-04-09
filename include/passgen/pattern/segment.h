#pragma once
#include "passgen/array.h"
#include "substring.h"

struct passgen_pattern_item;

typedef struct passgen_pattern_segment {
  struct passgen_pattern_substring pos;

  // array of segment_items
  passgen_array_t items;

  // struct pattern_repeat repeat;
} passgen_pattern_segment_t;

void passgen_pattern_segment_init(struct passgen_pattern_segment *segment);
void passgen_pattern_segment_free(struct passgen_pattern_segment *segment);

struct passgen_pattern_item *passgen_pattern_segment_new_item(struct passgen_pattern_segment *segment);

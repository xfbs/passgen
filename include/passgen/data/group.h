#pragma once
#include "passgen/container/stack/segment.h"
#include "passgen/data/repeat.h"
#include "passgen/data/substring.h"

typedef struct passgen_pattern_group {
  struct passgen_pattern_substring pos;

  // array of segments
  struct passgen_pattern_segment_stack segments;

  struct passgen_pattern_repeat repeat;
} passgen_pattern_group_t;

void passgen_pattern_group_init(passgen_pattern_group_t *group);
void passgen_pattern_group_free(passgen_pattern_group_t *group);

struct passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group_t *group);

struct passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group_t *group, size_t n);

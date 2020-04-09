#include "passgen/pattern/segments.h"
#include "passgen/pattern/segment.h"

void passgen_pattern_segments_init(struct passgen_pattern_segments *segments) {
  passgen_array_init(
      &segments->items,
      sizeof(struct passgen_pattern_segment),
      NULL);
}

struct passgen_pattern_segment *passgen_pattern_segments_new_segment(struct passgen_pattern_segments *segments) {
  return NULL;
}

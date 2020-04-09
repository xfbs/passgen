#include "passgen/pattern/segment.h"
#include "passgen/pattern/item.h"

void passgen_pattern_segment_init(struct passgen_pattern_segment *segment) {
  passgen_array_init(
      &segment->items,
      sizeof(struct passgen_pattern_item),
      NULL);
}

struct passgen_pattern_item *passgen_pattern_segment_new_item(struct passgen_pattern_segment *segment) {
  return NULL;
}

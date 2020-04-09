#include "passgen/pattern/group.h"
#include "passgen/pattern/item.h"
#include "passgen/pattern/segment.h"

void passgen_pattern_group_init(passgen_pattern_group_t *group) {
  passgen_array_init(
      &group->segments,
      sizeof(passgen_pattern_segment_t),
      NULL);
}

struct passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group_t *group) {
  struct passgen_pattern_segment *segment = passgen_array_push(
      &group->segments,
      sizeof(passgen_pattern_segment_t),
      NULL);
  passgen_pattern_segment_init(segment);

  return segment;
}

void passgen_pattern_group_free(passgen_pattern_group_t *group) {
}

#include "passgen/pattern/group.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/segments.h"

void passgen_pattern_group_init(passgen_pattern_group_t *group) {
  passgen_array_init(
      &group->segments,
      sizeof(passgen_pattern_segments_t),
      NULL);
}

struct passgen_pattern_segments *
passgen_pattern_group_add(passgen_pattern_group_t *group) {
  struct passgen_pattern_segments *segments = passgen_array_push(
      &group->segments,
      sizeof(passgen_pattern_segments_t),
      NULL);
  passgen_pattern_segments_init(segments);

  return segments;
}

void passgen_pattern_group_free(passgen_pattern_group_t *group) {
}

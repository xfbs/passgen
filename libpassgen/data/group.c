#include "passgen/data/group.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"

void passgen_pattern_group_init(passgen_pattern_group_t *group) {
  passgen_array_init(&group->segments, sizeof(passgen_pattern_segment_t), NULL);
}

void passgen_pattern_group_free(passgen_pattern_group_t *group) {
  for(size_t i = 0; i < group->segments.len; i++) {
    passgen_pattern_segment_t *segment = passgen_array_get(
        &group->segments,
        sizeof(passgen_pattern_segment_t),
        i);

    passgen_pattern_segment_free(segment);
  }

  passgen_array_free(&group->segments, sizeof(passgen_pattern_segment_t), NULL);
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

struct passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group_t *group, size_t n) {
  return passgen_array_get(
      &group->segments,
      sizeof(passgen_pattern_segment_t),
      n);
}

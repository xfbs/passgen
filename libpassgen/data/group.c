#include "passgen/data/group.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"

void passgen_pattern_group_init(passgen_pattern_group_t *group) {
  passgen_pattern_segment_stack_init(&group->segments, NULL);
}

void passgen_pattern_group_free(passgen_pattern_group_t *group) {
  for(size_t i = 0; i < group->segments.len; i++) {
    passgen_pattern_segment_t *segment = passgen_pattern_segment_stack_get(
        &group->segments,
        i);

    passgen_pattern_segment_free(segment);
  }

  passgen_pattern_segment_stack_free(&group->segments, NULL);
}

struct passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group_t *group) {
  struct passgen_pattern_segment *segment = passgen_pattern_segment_stack_push(
      &group->segments,
      NULL);
  passgen_pattern_segment_init(segment);

  return segment;
}

struct passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group_t *group, size_t n) {
  return passgen_pattern_segment_stack_get(
      &group->segments,
      n);
}

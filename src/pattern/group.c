#include "passgen/pattern/group.h"

#include "passgen/pattern/segment.h"

void passgen_pattern_group_init(passgen_pattern_group_t *group) {
    passgen_stack_init(&group->segments, sizeof(passgen_pattern_segment_t));
}

void passgen_pattern_group_free(passgen_pattern_group_t *group) {
    for(size_t i = 0; i < group->segments.len; i++) {
        passgen_pattern_segment_t *segment =
            passgen_stack_get(&group->segments, i);

        passgen_pattern_segment_free(segment);
    }

    passgen_stack_free(&group->segments);
}

struct passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group_t *group) {
    struct passgen_pattern_segment *segment =
        passgen_stack_push(&group->segments, NULL);
    passgen_pattern_segment_init(segment);

    return segment;
}

struct passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group_t *group, size_t n) {
    return passgen_stack_get(&group->segments, n);
}

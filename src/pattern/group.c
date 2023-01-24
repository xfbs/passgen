#include "passgen/pattern/group.h"

#include "passgen/pattern/segment.h"

void passgen_pattern_group_init(passgen_pattern_group *group) {
    passgen_stack_init(&group->segments, sizeof(passgen_pattern_segment));
    group->multiplier_sum = 0;
}

void passgen_pattern_group_free(passgen_pattern_group *group) {
    for(size_t i = 0; i < group->segments.len; i++) {
        passgen_pattern_segment *segment =
            passgen_stack_get(&group->segments, i);

        passgen_pattern_segment_free(segment);
    }

    passgen_stack_free(&group->segments);
}

void passgen_pattern_group_finish(passgen_pattern_group *group) {
    group->multiplier_sum = 0;
    for(size_t i = 0; i < group->segments.len; i++) {
        passgen_pattern_segment *segment =
            passgen_stack_get(&group->segments, i);
        group->multiplier_sum += segment->multiplier;
    }

    // TODO: sort segments by multiplier (highest first) for more efficient
    // generation.
}

passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group *group) {
    group->multiplier_sum += 1;
    passgen_pattern_segment *segment =
        passgen_stack_push(&group->segments, NULL);
    passgen_pattern_segment_init(segment);

    return segment;
}

passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group *group, size_t n) {
    return passgen_stack_get(&group->segments, n);
}

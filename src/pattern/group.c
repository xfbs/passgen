#include "passgen/pattern/group.h"
#include "passgen/pattern/segment.h"
#include "passgen/config.h"

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

    PASSGEN_CLEAR(group);
}

void passgen_pattern_group_finish(passgen_pattern_group *group) {
    // iterate through the segments and find out if there are any non-empty
    // ones.
    bool empty = true;
    for(size_t i = 0; i < group->segments.len; i++) {
        passgen_pattern_segment *segment =
            passgen_stack_get(&group->segments, i);

        // make this as non-empty only if it has items and a non-zero
        // multiplier. zero multiplier segments are removed during parsing,
        // so we can assume that all segments have a positive multiplier.
        if(segment->items.len > 0) {
            empty = false;
        }
    }

    // if no non-empty segments were found, set the multiplier sum to zero to
    // indicate that this group can be removed.
    if(empty) {
        group->multiplier_sum = 0;
    }
}

passgen_pattern_segment *
passgen_pattern_group_segment_append(passgen_pattern_group *group) {
    group->multiplier_sum += 1;
    passgen_pattern_segment *segment =
        passgen_stack_push(&group->segments, NULL);
    passgen_pattern_segment_init(segment);

    return segment;
}

passgen_pattern_segment *
passgen_pattern_group_segment_get(passgen_pattern_group *group, size_t n) {
    return passgen_stack_get(&group->segments, n);
}

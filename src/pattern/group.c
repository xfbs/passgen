#include "passgen/pattern/group.h"
#include "passgen/config.h"
#include "passgen/pattern/segment.h"
#include "passgen/util/clear.h"

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

passgen_pattern_segment *
passgen_pattern_group_segment_append(passgen_pattern_group *group) {
    passgen_pattern_segment *segment =
        passgen_stack_push(&group->segments, NULL);
    if(!segment) {
        return NULL;
    }
    group->multiplier_sum += 1;
    passgen_pattern_segment_init(segment);

    return segment;
}

passgen_pattern_segment *
passgen_pattern_group_segment_get(passgen_pattern_group *group, size_t n) {
    return passgen_stack_get(&group->segments, n);
}

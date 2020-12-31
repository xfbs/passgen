#include "passgen/data/group.h"

#include "passgen/data/segment.h"

void passgen_pattern_group_init(passgen_pattern_group_t *group) {
    passgen_pattern_segment_stack_init(&group->segments, NULL);
}

void passgen_pattern_group_free(passgen_pattern_group_t *group) {
    for(size_t i = 0; i < group->segments.len; i++) {
        passgen_pattern_segment_t *segment =
            passgen_pattern_segment_stack_get(&group->segments, i);

        passgen_pattern_segment_free(segment);
    }

    passgen_pattern_segment_stack_free(&group->segments, NULL);
}

struct passgen_pattern_segment *
passgen_pattern_group_new_segment(passgen_pattern_group_t *group) {
    struct passgen_pattern_segment *segment =
        passgen_pattern_segment_stack_push(&group->segments, NULL);
    passgen_pattern_segment_init(segment);

    return segment;
}

struct passgen_pattern_segment *
passgen_pattern_group_get_segment(passgen_pattern_group_t *group, size_t n) {
    return passgen_pattern_segment_stack_get(&group->segments, n);
}

void passgen_pattern_group_debug(passgen_pattern_group_t *group, passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_pattern_group");

    debug->member_start(debug->data, "segments");
    debug->array_start(debug->data);

    for(size_t i = 0; i < group->segments.len; i++) {
        if(i != 0) {
            debug->array_sep(debug->data);
        }

        passgen_pattern_segment_t *segment =
            passgen_pattern_segment_stack_get(&group->segments, i);

        passgen_pattern_segment_debug(segment, debug);
    }

    debug->array_end(debug->data);
    debug->member_end(debug->data, "segments");

    debug->struct_end(debug->data, "passgen_pattern_group");
}

int passgen_group_export(passgen_pattern_group_t *group, void *data, passgen_export_cb *fn) {
    fn(data, '(');

    for(size_t i = 0; i < group->segments.len; i++) {
        if(i != 0) {
            fn(data, '|');
        }

        passgen_pattern_segment_t *segment =
            passgen_pattern_segment_stack_get(&group->segments, i);

        int export_return = passgen_segment_export(segment, data, fn);
        if(export_return != PASSGEN_EXPORT_SUCCESS) {
            return export_return;
        }
    }

    fn(data, ')');
    return PASSGEN_EXPORT_SUCCESS;
}

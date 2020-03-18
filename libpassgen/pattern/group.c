#include "passgen/pattern/group.h"
#include "passgen/pattern/segments.h"

void passgen_pattern_group_init(passgen_pattern_group_t *group) {
    passgen_array_init(&group->segments, sizeof(passgen_pattern_segments_t), NULL);
}

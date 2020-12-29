#include "passgen/data/segment.h"

#include "passgen/container/stack/segment_item.h"
#include "passgen/data/group.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"

void passgen_pattern_segment_init(struct passgen_pattern_segment *segment) {
    passgen_pattern_item_stack_init(&segment->items, NULL);
}

void passgen_pattern_segment_free(struct passgen_pattern_segment *segment) {
    for(size_t i = 0; i < segment->items.len; i++) {
        struct passgen_pattern_item *item =
            passgen_pattern_item_stack_get(&segment->items, i);

        passgen_pattern_item_free(item);
    }

    passgen_pattern_item_stack_free(&segment->items, NULL);
}

struct passgen_pattern_item *
passgen_pattern_segment_new_item(struct passgen_pattern_segment *segment) {
    struct passgen_pattern_item *item =
        passgen_pattern_item_stack_push(&segment->items, NULL);
    passgen_pattern_item_init(item);

    return item;
}

struct passgen_pattern_char *
passgen_pattern_segment_new_char(struct passgen_pattern_segment *segment) {
    struct passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_CHAR;
    item->data.character.count = 0;
    item->data.character.tainted = 0;

    return &item->data.character;
}

struct passgen_pattern_set *
passgen_pattern_segment_new_set(struct passgen_pattern_segment *segment) {
    struct passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_SET;
    passgen_pattern_set_init(&item->data.set);

    return &item->data.set;
}

struct passgen_pattern_group *
passgen_pattern_segment_new_group(struct passgen_pattern_segment *segment) {
    struct passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_GROUP;
    passgen_pattern_group_init(&item->data.group);

    return &item->data.group;
}

struct passgen_pattern_special *
passgen_pattern_segment_new_special(struct passgen_pattern_segment *segment) {
    struct passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_SPECIAL;

    return &item->data.special;
}

struct passgen_pattern_item *passgen_pattern_segment_get_item(
    struct passgen_pattern_segment *segment,
    size_t n) {
    return passgen_pattern_item_stack_get(&segment->items, n);
}

void passgen_pattern_segment_debug(passgen_pattern_segment_t *segment, passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_pattern_segment");

    /*
    debug->member_start(debug->data, "segments");
    debug->array_start(debug->data);

    for(size_t i = 0; i < group->segments.len; i++) {
        passgen_pattern_segment_t *segment =
            passgen_pattern_segment_stack_get(&group->segments, i);

        passgen_pattern_segment_debug(segment, debug);
    }

    debug->array_end(debug->data);
    debug->member_end(debug->data, "segments");
    */

    debug->struct_end(debug->data, "passgen_pattern_segment");
}

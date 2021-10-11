#include "passgen/data/segment.h"

#include "passgen/container/stack_new.h"
#include "passgen/data/group.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"

void passgen_pattern_segment_init(struct passgen_pattern_segment *segment) {
    passgen_stack_init(&segment->items, sizeof(passgen_pattern_item_t));
}

void passgen_pattern_segment_free(struct passgen_pattern_segment *segment) {
    for(size_t i = 0; i < segment->items.len; i++) {
        struct passgen_pattern_item *item =
            passgen_stack_get(&segment->items, i);

        passgen_pattern_item_free(item);
    }

    passgen_stack_free(&segment->items);
}

struct passgen_pattern_item *
passgen_pattern_segment_new_item(struct passgen_pattern_segment *segment) {
    struct passgen_pattern_item *item =
        passgen_stack_push(&segment->items, NULL);
    passgen_pattern_item_init(item);

    return item;
}

struct passgen_chars *
passgen_pattern_segment_new_char(struct passgen_pattern_segment *segment) {
    struct passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_CHAR;
    item->data.chars.count = 0;
    item->data.chars.tainted = 0;

    return &item->data.chars;
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
    return passgen_stack_get(&segment->items, n);
}

void passgen_pattern_segment_debug(
    passgen_pattern_segment_t *segment,
    passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_pattern_segment");

    debug->member_start(debug->data, "items");
    debug->array_start(debug->data);

    for(size_t i = 0; i < segment->items.len; i++) {
        if(i != 0) {
            debug->array_sep(debug->data);
        }

        struct passgen_pattern_item *item =
            passgen_stack_get(&segment->items, i);

        passgen_pattern_item_debug(item, debug);
    }

    debug->array_end(debug->data);
    debug->member_end(debug->data, "items");

    debug->struct_end(debug->data, "passgen_pattern_segment");
}

int passgen_segment_export(
    passgen_pattern_segment_t *segment,
    void *data,
    passgen_export_cb *fn) {
    for(size_t i = 0; i < segment->items.len; i++) {
        struct passgen_pattern_item *item =
            passgen_stack_get(&segment->items, i);

        int export_return = passgen_item_export(item, data, fn);

        // return early on error
        if(export_return != PASSGEN_EXPORT_SUCCESS) {
            return export_return;
        }
    }

    return PASSGEN_EXPORT_SUCCESS;
}

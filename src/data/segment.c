#include "passgen/data/segment.h"
#include "passgen/data/group.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"
#include "passgen/stack.h"

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

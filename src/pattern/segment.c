#include "passgen/pattern/segment.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/pattern_kind.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/set.h"
#include "passgen/util/stack.h"

void passgen_pattern_segment_init(passgen_pattern_segment *segment) {
    segment->multiplier = 1;
    passgen_stack_init(&segment->items, sizeof(passgen_pattern_item));
}

void passgen_pattern_segment_free(passgen_pattern_segment *segment) {
    for(size_t i = 0; i < segment->items.len; i++) {
        passgen_pattern_item *item =
            passgen_stack_get(&segment->items, i);

        passgen_pattern_item_free(item);
    }

    passgen_stack_free(&segment->items);
}

passgen_pattern_item *
passgen_pattern_segment_new_item(passgen_pattern_segment *segment) {
    passgen_pattern_item *item =
        passgen_stack_push(&segment->items, NULL);
    passgen_pattern_item_init(item);

    return item;
}

passgen_pattern_literal *
passgen_pattern_segment_new_char(passgen_pattern_segment *segment) {
    passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_CHAR;
    item->data.chars.count = 0;
    item->data.chars.tainted = 0;

    return &item->data.chars;
}

passgen_pattern_set *
passgen_pattern_segment_new_set(passgen_pattern_segment *segment) {
    passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_SET;
    passgen_pattern_set_init(&item->data.set);

    return &item->data.set;
}

passgen_pattern_group *
passgen_pattern_segment_new_group(passgen_pattern_segment *segment) {
    passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_GROUP;
    passgen_pattern_group_init(&item->data.group);

    return &item->data.group;
}

passgen_pattern_special *
passgen_pattern_segment_new_special(passgen_pattern_segment *segment) {
    passgen_pattern_item *item =
        passgen_pattern_segment_new_item(segment);
    item->kind = PASSGEN_PATTERN_SPECIAL;

    return &item->data.special;
}

passgen_pattern_item *passgen_pattern_segment_get_item(
    passgen_pattern_segment *segment,
    size_t n) {
    return passgen_stack_get(&segment->items, n);
}

#include "passgen/pattern/segment_item.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"

void passgen_pattern_item_free(struct passgen_pattern_item *item) {
    switch(item->kind) {
        case PASSGEN_PATTERN_SET:
            passgen_pattern_set_free(&item->data.set);
            break;
        case PASSGEN_PATTERN_GROUP:
            passgen_pattern_group_free(&item->data.group);
            break;
        case PASSGEN_PATTERN_SPECIAL:
            passgen_pattern_special_free(&item->data.special);
            break;
        default:
            break;
    }
}

void passgen_pattern_item_init(struct passgen_pattern_item *item) {
    item->repeat.min = 1;
    item->repeat.max = 1;
    item->maybe = 0;
}
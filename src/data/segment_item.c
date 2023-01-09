#include "passgen/data/segment_item.h"
#include "passgen/data/group.h"
#include "passgen/data/set.h"
#include "passgen/data/special.h"

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

int passgen_item_export(
    passgen_pattern_item_t *item,
    void *data,
    passgen_export_cb *fn) {
    switch(item->kind) {
        case PASSGEN_PATTERN_SET:
            return passgen_charset_export(&item->data.set, data, fn);
        case PASSGEN_PATTERN_GROUP:
            return passgen_group_export(&item->data.group, data, fn);
        case PASSGEN_PATTERN_CHAR:
            return passgen_chars_export(&item->data.chars, data, fn);
        case PASSGEN_PATTERN_SPECIAL:
            break;
    }

    return PASSGEN_EXPORT_FAILURE;
}

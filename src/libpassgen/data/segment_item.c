#include "passgen/data/segment_item.h"
#include "passgen/data/group.h"
#include "passgen/data/set.h"

void passgen_pattern_item_free(struct passgen_pattern_item *item) {
    switch(item->kind) {
        case PASSGEN_PATTERN_SET:
            passgen_pattern_set_free(&item->data.set);
            break;
        case PASSGEN_PATTERN_GROUP:
            passgen_pattern_group_free(&item->data.group);
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

void passgen_pattern_item_debug(
    passgen_pattern_item_t *item,
    passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_pattern_item");

    debug->member_start(debug->data, "kind");
    switch(item->kind) {
        case PASSGEN_PATTERN_SET:
            debug->value_enum(debug->data, item->kind, "SET");
            break;
        case PASSGEN_PATTERN_GROUP:
            debug->value_enum(debug->data, item->kind, "GROUP");
            break;
        case PASSGEN_PATTERN_CHAR:
            debug->value_enum(debug->data, item->kind, "CHAR");
            break;
        case PASSGEN_PATTERN_SPECIAL:
            debug->value_enum(debug->data, item->kind, "SPECIAL");
            break;
    }
    debug->member_end(debug->data, "kind");

    debug->member_start(debug->data, "data");
    switch(item->kind) {
        case PASSGEN_PATTERN_SET:
            passgen_pattern_set_debug(&item->data.set, debug);
            break;
        case PASSGEN_PATTERN_GROUP:
            passgen_pattern_group_debug(&item->data.group, debug);
            break;
        case PASSGEN_PATTERN_CHAR:
            passgen_pattern_char_debug(&item->data.chars, debug);
            break;
        case PASSGEN_PATTERN_SPECIAL:
            break;
    }
    debug->member_end(debug->data, "data");

    debug->member_start(debug->data, "repeat");
    passgen_pattern_repeat_debug(&item->repeat, debug);
    debug->member_end(debug->data, "repeat");

    debug->member_start(debug->data, "maybe");
    debug->value_bool(debug->data, item->maybe);
    debug->member_end(debug->data, "maybe");

    debug->struct_end(debug->data, "passgen_pattern_item");
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

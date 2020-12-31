#pragma once
#include <stdbool.h>

#include "passgen/container/stack.h"
#include "passgen/data/chars.h"
#include "passgen/data/group.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/repeat.h"
#include "passgen/data/set.h"
#include "passgen/data/special.h"
#include "passgen/debug.h"

typedef struct passgen_pattern_item {
    enum passgen_pattern_kind kind;

    union {
        struct passgen_pattern_set set;
        struct passgen_pattern_group group;
        passgen_chars_t chars;
        struct passgen_pattern_special special;
    } data;

    struct passgen_pattern_repeat repeat;
    bool maybe;
} passgen_pattern_item_t;

void passgen_pattern_item_init(struct passgen_pattern_item *item);
void passgen_pattern_item_free(struct passgen_pattern_item *item);

void passgen_pattern_item_debug(passgen_pattern_item_t *item, passgen_debug_t *debug);

int passgen_item_export(passgen_pattern_item_t *item, void *data, passgen_export_cb *fn);

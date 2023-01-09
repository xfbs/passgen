#pragma once
#include <stdbool.h>

#include "passgen/util/stack.h"
#include "passgen/pattern/chars.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/pattern_kind.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"

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
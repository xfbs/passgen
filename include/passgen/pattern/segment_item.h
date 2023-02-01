#pragma once
#include <stdbool.h>

#include "passgen/container/stack.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/pattern_kind.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"

typedef struct passgen_pattern_item {
    enum passgen_pattern_kind kind;

    union {
        passgen_pattern_set set;
        passgen_pattern_group group;
        passgen_pattern_literal chars;
        passgen_pattern_special special;
    } data;

    struct passgen_pattern_repeat repeat;
    bool maybe;
} passgen_pattern_item;

void passgen_pattern_item_init(passgen_pattern_item *item);
void passgen_pattern_item_free(passgen_pattern_item *item);

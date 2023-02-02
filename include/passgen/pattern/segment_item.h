/// @file segment_item.h
/// @author Patrick M. Elsen <pelsen@xfbs.net>
/// @brief Pattern segment item (such as the `[a-z]` in `[a-z][a-z]`)
#pragma once
#include <stdbool.h>

#include "passgen/container/stack.h"
#include "passgen/pattern/literal.h"
#include "passgen/pattern/group.h"
#include "passgen/pattern/repeat.h"
#include "passgen/pattern/set.h"
#include "passgen/pattern/special.h"

/// Kind of segment item.
enum passgen_pattern_kind {
    PASSGEN_PATTERN_SET,
    PASSGEN_PATTERN_CHAR,
    PASSGEN_PATTERN_SPECIAL,
    PASSGEN_PATTERN_GROUP,
};

/// Passgen segment item. This can house any other kind of passgen syntax
/// element. A segment is an array of segment items.
typedef struct passgen_pattern_item {
    /// What kind this item is.
    enum passgen_pattern_kind kind;

    union {
        passgen_pattern_set set;
        passgen_pattern_group group;
        passgen_pattern_literal chars;
        passgen_pattern_special special;
    } data;

    /// Repetition of this segment item.
    struct passgen_pattern_repeat repeat;

    /// Marks this segment item as optional
    bool maybe;
} passgen_pattern_item;

/// Allocate a new segment item
void passgen_pattern_item_init(passgen_pattern_item *item);

/// Release segment item memory
void passgen_pattern_item_free(passgen_pattern_item *item);

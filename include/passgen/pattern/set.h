#pragma once
#include <stddef.h>

#include "passgen/container/stack.h"
#include "passgen/container/array.h"
#include "passgen/pattern/repeat.h"

struct passgen_pattern_range;

/// A set represents a set of possible characters.
///
/// For example, `[a-z0-9]` is a set expression that can produce lowercase latin
/// characters of numbers.
typedef struct passgen_pattern_set {
    // array of range_items
    passgen_stack items;

    // how many choices to choose from. for efficient generation. improved
    // locality.
    size_t *choices_list;
} passgen_pattern_set;

void passgen_pattern_set_init(passgen_pattern_set *set);
void passgen_pattern_set_free(passgen_pattern_set *set);

struct passgen_pattern_range *
passgen_pattern_set_new_range(passgen_pattern_set *set);

struct passgen_pattern_range *
passgen_pattern_set_get_range(passgen_pattern_set *set, size_t n);

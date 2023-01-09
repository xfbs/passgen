#pragma once
#include <stddef.h>

#include "passgen/stack.h"
#include "passgen/data/array.h"
#include "passgen/data/repeat.h"

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
} passgen_pattern_set_t;

void passgen_pattern_set_init(struct passgen_pattern_set *set);
void passgen_pattern_set_free(struct passgen_pattern_set *set);

struct passgen_pattern_range *
passgen_pattern_set_new_range(struct passgen_pattern_set *set);

struct passgen_pattern_range *
passgen_pattern_set_get_range(struct passgen_pattern_set *set, size_t n);

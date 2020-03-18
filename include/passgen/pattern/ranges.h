#pragma once
#include "substring.h"
#include "repeat.h"
#include "passgen/array.h"

struct passgen_pattern_ranges {
    struct passgen_pattern_substring pos;

    // array of range_items
    passgen_array_t items;

    // how many choices to choose from. for efficient generation. improved
    // locality.
    size_t *choices_list;

    struct passgen_pattern_repeat repeat;
};

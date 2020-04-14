#pragma once
#include "passgen/container/stack/range.h"
#include "passgen/data/array.h"
#include "passgen/data/repeat.h"
#include "passgen/data/substring.h"

struct passgen_pattern_range;

struct passgen_pattern_set {
  struct passgen_pattern_substring pos;

  // array of range_items
  struct passgen_pattern_range_stack items;

  // how many choices to choose from. for efficient generation. improved
  // locality.
  size_t *choices_list;

  struct passgen_pattern_repeat repeat;
};

void passgen_pattern_set_init(struct passgen_pattern_set *set);
void passgen_pattern_set_free(struct passgen_pattern_set *set);

struct passgen_pattern_range *
passgen_pattern_set_new_range(struct passgen_pattern_set *set);

struct passgen_pattern_range *
passgen_pattern_set_get_range(struct passgen_pattern_set *set, size_t n);

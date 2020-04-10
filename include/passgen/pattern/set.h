#pragma once
#include "passgen/data/array.h"
#include "repeat.h"
#include "substring.h"

struct passgen_pattern_range;

struct passgen_pattern_set {
  struct passgen_pattern_substring pos;

  // array of range_items
  passgen_array_t items;

  // how many choices to choose from. for efficient generation. improved
  // locality.
  size_t *choices_list;

  struct passgen_pattern_repeat repeat;
};

void passgen_pattern_set_init(struct passgen_pattern_set *set);
void passgen_pattern_set_free(struct passgen_pattern_set *set);

struct passgen_pattern_range *
passgen_pattern_set_new_range(struct passgen_pattern_set *set);

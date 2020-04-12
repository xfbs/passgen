#include "passgen/data/set.h"
#include "passgen/data/range.h"

void passgen_pattern_set_init(struct passgen_pattern_set *set) {
  passgen_pattern_range_stack_init(&set->items, NULL);

  set->choices_list = NULL;
}

void passgen_pattern_set_free(struct passgen_pattern_set *set) {
  passgen_pattern_range_stack_free(&set->items, NULL);
}

struct passgen_pattern_range *
passgen_pattern_set_new_range(struct passgen_pattern_set *set) {
  return passgen_pattern_range_stack_push(&set->items, NULL);
}

struct passgen_pattern_range *
passgen_pattern_set_get_range(struct passgen_pattern_set *set, size_t n) {
  return passgen_pattern_range_stack_get(&set->items, n);
}

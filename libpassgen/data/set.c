#include "passgen/pattern/set.h"
#include "passgen/pattern/range.h"

void passgen_pattern_set_init(struct passgen_pattern_set *set) {
  passgen_array_init(&set->items, sizeof(struct passgen_pattern_range), NULL);

  set->choices_list = NULL;
}

void passgen_pattern_set_free(struct passgen_pattern_set *set) {
  passgen_array_free(&set->items, sizeof(struct passgen_pattern_range), NULL);
}

struct passgen_pattern_range *
passgen_pattern_set_new_range(struct passgen_pattern_set *set) {
  return passgen_array_push(
      &set->items,
      sizeof(struct passgen_pattern_range),
      NULL);
}

struct passgen_pattern_range *
passgen_pattern_set_get_range(struct passgen_pattern_set *set, size_t n) {
  return passgen_array_get(
      &set->items,
      sizeof(struct passgen_pattern_range),
      n);
}

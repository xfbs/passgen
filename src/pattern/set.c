#include "passgen/pattern/set.h"
#include "passgen/pattern/range.h"
#include <stdlib.h>

void passgen_pattern_set_init(struct passgen_pattern_set *set) {
    passgen_stack_init(&set->items, sizeof(passgen_pattern_range_t));

    set->choices_list = NULL;
}

void passgen_pattern_set_free(struct passgen_pattern_set *set) {
    passgen_stack_free(&set->items);

    free(set->choices_list);
}

struct passgen_pattern_range *
passgen_pattern_set_new_range(struct passgen_pattern_set *set) {
    return passgen_stack_push(&set->items, NULL);
}

struct passgen_pattern_range *
passgen_pattern_set_get_range(struct passgen_pattern_set *set, size_t n) {
    return passgen_stack_get(&set->items, n);
}

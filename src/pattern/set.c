#include "passgen/pattern/set.h"
#include "passgen/config.h"
#include "passgen/pattern/range.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"

void passgen_pattern_set_init(passgen_pattern_set *set) {
    passgen_stack_init(&set->items, sizeof(passgen_pattern_range));

    set->choices_list = NULL;
}

void passgen_pattern_set_free(passgen_pattern_set *set) {
    // the list of choices has an entry for every range, once the set is
    // complete.
    passgen_free(set->choices_list, set->items.len * sizeof(size_t));
    passgen_stack_free(&set->items);
    PASSGEN_CLEAR(set);
}

passgen_pattern_range *
passgen_pattern_set_range_append(passgen_pattern_set *set) {
    return passgen_stack_push(&set->items, NULL);
}

passgen_pattern_range *
passgen_pattern_set_range_get(passgen_pattern_set *set, size_t n) {
    return passgen_stack_get(&set->items, n);
}

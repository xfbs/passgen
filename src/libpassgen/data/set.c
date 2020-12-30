#include "passgen/data/set.h"
#include "passgen/data/range.h"
#include "passgen/memory.h"

void passgen_pattern_set_init(struct passgen_pattern_set *set) {
    passgen_pattern_range_stack_init(&set->items, NULL);

    set->choices_list = NULL;
}

void passgen_pattern_set_free(struct passgen_pattern_set *set) {
    passgen_pattern_range_stack_free(&set->items, NULL);

    passgen_free(NULL, set->choices_list);
}

struct passgen_pattern_range *
passgen_pattern_set_new_range(struct passgen_pattern_set *set) {
    return passgen_pattern_range_stack_push(&set->items, NULL);
}

struct passgen_pattern_range *
passgen_pattern_set_get_range(struct passgen_pattern_set *set, size_t n) {
    return passgen_pattern_range_stack_get(&set->items, n);
}

void passgen_pattern_set_debug(passgen_pattern_set_t *set, passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_pattern_set");

    debug->member_start(debug->data, "items");
    debug->array_start(debug->data);
    for(size_t i = 0; i < set->items.len; i++) {
        if(i != 0) {
            debug->array_sep(debug->data);
        }

        struct passgen_pattern_range *range = passgen_pattern_set_get_range(set, i);
        passgen_pattern_range_debug(range, debug);
    }
    debug->array_end(debug->data);
    debug->member_end(debug->data, "items");

    debug->member_start(debug->data, "choices_list");
    debug->value_pointer(debug->data, set->choices_list);
    debug->member_end(debug->data, "choices_list");

    debug->struct_end(debug->data, "passgen_pattern_set");
}

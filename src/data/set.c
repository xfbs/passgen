#include "passgen/data/set.h"
#include "passgen/data/range.h"
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

int passgen_charset_export(
    passgen_pattern_set_t *set,
    void *data,
    passgen_export_cb *fn) {
    fn(data, '[');

    for(size_t i = 0; i < set->items.len; i++) {
        struct passgen_pattern_range *range =
            passgen_pattern_set_get_range(set, i);
        passgen_pattern_range_export(range, data, fn);
    }

    fn(data, ']');

    return PASSGEN_EXPORT_SUCCESS;
}

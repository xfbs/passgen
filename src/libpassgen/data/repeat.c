#include "passgen/data/repeat.h"

void passgen_pattern_repeat_debug(passgen_repeat_t *repeat, passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_repeat");

    debug->member_start(debug->data, "min");
    debug->value_uint(debug->data, repeat->min);
    debug->member_end(debug->data, "min");

    debug->member_start(debug->data, "max");
    debug->value_uint(debug->data, repeat->max);
    debug->member_end(debug->data, "max");

    debug->struct_end(debug->data, "passgen_repeat");
}

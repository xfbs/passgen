#include "passgen/data/range.h"

void passgen_pattern_range_debug(passgen_pattern_range_t *range, passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_pattern_range");

    debug->member_start(debug->data, "start");
    debug->value_uint(debug->data, range->start);
    debug->member_end(debug->data, "start");

    debug->member_start(debug->data, "end");
    debug->value_uint(debug->data, range->end);
    debug->member_end(debug->data, "end");

    debug->member_start(debug->data, "multiplier");
    debug->value_uint(debug->data, range->multiplier);
    debug->member_end(debug->data, "multiplier");

    debug->struct_end(debug->data, "passgen_pattern_range");
}

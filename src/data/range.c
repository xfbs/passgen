#include "passgen/data/range.h"

int passgen_pattern_range_export(
    passgen_pattern_range_t *range,
    void *data,
    passgen_export_cb *fn) {
    if(range->start == range->end) {
        fn(data, range->start);
    } else {
        fn(data, range->start);
        fn(data, '-');
        fn(data, range->end);
    }

    return PASSGEN_EXPORT_SUCCESS;
}

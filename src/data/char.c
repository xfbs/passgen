#include "passgen/data/chars.h"

int passgen_chars_export(
    passgen_chars_t *chars,
    void *data,
    passgen_export_cb *fn) {
    for(size_t i = 0; i < chars->count; i++) {
        fn(data, chars->codepoints[i]);
    }

    return PASSGEN_EXPORT_SUCCESS;
}

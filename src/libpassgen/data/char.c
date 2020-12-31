#include "passgen/data/chars.h"

void passgen_pattern_char_debug(passgen_chars_t *character, passgen_debug_t *debug) {
    debug->struct_start(debug->data, "passgen_pattern_char");

    debug->member_start(debug->data, "count");
    debug->value_uint(debug->data, character->count);
    debug->member_end(debug->data, "count");

    debug->member_start(debug->data, "tainted");
    debug->value_bool(debug->data, character->tainted);
    debug->member_end(debug->data, "tainted");

    debug->member_start(debug->data, "codepoints");
    debug->array_start(debug->data);
    for(size_t i = 0; i < character->count; i++) {
        if(i != 0) {
            debug->array_sep(debug->data);
        }

        debug->value_uint(debug->data, character->codepoints[i]);
    }
    debug->array_end(debug->data);
    debug->member_end(debug->data, "codepoints");

    debug->struct_end(debug->data, "passgen_pattern_char");
}

int passgen_chars_export(passgen_chars_t *chars, void *data, passgen_export_cb *fn) {
    for(size_t i = 0; i < chars->count; i++) {
        fn(data, chars->codepoints[i]);
    }

    return PASSGEN_EXPORT_SUCCESS;
}

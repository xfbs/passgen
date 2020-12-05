#include "passgen/data/special.h"
#include "passgen/assert.h"
#include <stdbool.h>

void passgen_pattern_special_init_char(
    struct passgen_pattern_special *special,
    uint32_t kind) {
    // set sane defaults for length.
    special->length.min = 0;
    special->length.max = 64;

    switch(kind) {
        case 'p':
            special->kind = PASSGEN_PATTERN_SPECIAL_PRONOUNCABLE;
            break;
        case 'w':
            special->kind = PASSGEN_PATTERN_SPECIAL_WORDLIST;
            break;
        default:
            assert(false);
            break;
    }
}

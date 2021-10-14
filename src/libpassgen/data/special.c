#include "passgen/data/special.h"
#include "passgen/assert.h"
#include <stdbool.h>

void passgen_pattern_special_init_char(
    struct passgen_pattern_special *special,
    uint32_t kind) {
    // set sane defaults for length.
    special->parameters_len = 0;
    special->parameters = calloc(256, sizeof(int32_t));

    switch(kind) {
        case 'p':
            special->kind = PASSGEN_PATTERN_SPECIAL_PRONOUNCABLE;
            break;
        case 'w':
            special->kind = PASSGEN_PATTERN_SPECIAL_WORDLIST;
            break;
        default:
            passgen_assert(false);
            break;
    }
}

void passgen_pattern_special_free(struct passgen_pattern_special *special) {
    free(special->parameters);
}

void passgen_pattern_special_add_parameter_cp(
    struct passgen_pattern_special *special,
    int32_t codepoint) {
    special->parameters[special->parameters_len] = codepoint;
    special->parameters_len++;
}

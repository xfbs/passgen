#include "passgen/pattern/special.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/util/utf8.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define PARAMETERS_INITIAL_SIZE 16
#define PARAMETERS_MULTIPLIER   2

void passgen_pattern_special_init(
    passgen_pattern_special *special,
    uint32_t kind) {
    special->parameters_len = 0;
    special->parameters_cap = 0;
    special->parameters = NULL;

    switch(kind) {
        case 'm':
            special->kind = PASSGEN_PATTERN_SPECIAL_MARKOV;
            break;
        case 'w':
            special->kind = PASSGEN_PATTERN_SPECIAL_WORDLIST;
            break;
        case 'p':
            special->kind = PASSGEN_PATTERN_SPECIAL_PRESET;
            break;
        default:
            passgen_assert(false);
            break;
    }
}

void passgen_pattern_special_free(passgen_pattern_special *special) {
    free(special->parameters);
    PASSGEN_CLEAR(special);
}

void passgen_pattern_special_push(
    passgen_pattern_special *special,
    int32_t codepoint) {
    if(special->parameters_cap == 0) {
        special->parameters_cap = PARAMETERS_INITIAL_SIZE;
        special->parameters = malloc(special->parameters_cap);
    }

    // always leave enough space for a full UTF8 character (4 bytes) plus a
    // NULL.
    if((special->parameters_len + 5) >= special->parameters_cap) {
        special->parameters_cap *= PARAMETERS_MULTIPLIER;
        special->parameters =
            realloc(special->parameters, special->parameters_cap);
    }

    int bytes = passgen_utf8_encode_codepoint(
        (uint8_t *) &special->parameters[special->parameters_len],
        codepoint);

    passgen_assert(bytes > 0);
    special->parameters_len += bytes;

    assert(special->parameters_len < special->parameters_cap);

    // always NULL-terminate the string.
    special->parameters[special->parameters_len] = 0;
}

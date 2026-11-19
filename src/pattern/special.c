#include "passgen/pattern/special.h"
#include "passgen/assert.h"
#include "passgen/config.h"
#include "passgen/markov.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"
#include "passgen/util/utf8.h"
#include <stdbool.h>

#define PARAMETERS_INITIAL_SIZE 16
#define PARAMETERS_MULTIPLIER   2

void passgen_pattern_special_init(
    passgen_pattern_special *special,
    uint32_t kind) {
    special->parameters_len = 0;
    special->parameters_cap = 0;
    special->parameters = NULL;
    special->markov_length = PASSGEN_MARKOV_LENGTH_DEFAULT;
    special->start = (passgen_error_offset){0, 0};
    special->end = (passgen_error_offset){0, 0};
    special->wordlist = NULL;

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
    passgen_free(special->parameters, special->parameters_cap);
    PASSGEN_CLEAR(special);
}

passgen_status passgen_pattern_special_push(
    passgen_pattern_special *special,
    int32_t codepoint) {
    // always leave enough space for a full UTF8 character (4 bytes) plus a
    // NULL.
    if((special->parameters_len + 5) >= special->parameters_cap) {
        int32_t capacity = special->parameters_cap
                               ? special->parameters_cap * PARAMETERS_MULTIPLIER
                               : PARAMETERS_INITIAL_SIZE;
        char *parameters = passgen_resize(
            special->parameters,
            special->parameters_cap,
            capacity);
        if(!parameters) {
            return PASSGEN_ERROR_OUT_OF_MEMORY;
        }
        special->parameters = parameters;
        special->parameters_cap = capacity;
    }

    int bytes = passgen_utf8_encode_codepoint(
        (uint8_t *) &special->parameters[special->parameters_len],
        codepoint);

    // codepoints which cannot be encoded are left out. adding the (negative)
    // error to the length would corrupt it.
    if(bytes <= 0) {
        return PASSGEN_OK;
    }

    special->parameters_len += bytes;
    passgen_assert(special->parameters_len < special->parameters_cap);

    // always NULL-terminate the string.
    special->parameters[special->parameters_len] = 0;

    return PASSGEN_OK;
}

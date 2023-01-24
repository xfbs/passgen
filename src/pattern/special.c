#include "passgen/pattern/special.h"
#include "passgen/assert.h"
#include <stdbool.h>
#include <stdlib.h>

#define PARAMETERS_INITIAL_SIZE 16
#define PARAMETERS_MULTIPLIER 2

void passgen_pattern_special_init_char(
    struct passgen_pattern_special *special,
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

    if(special->parameters_cap == 0) {
        special->parameters = calloc(PARAMETERS_INITIAL_SIZE, sizeof(int32_t));
        special->parameters_cap = PARAMETERS_INITIAL_SIZE;
    }

    if(special->parameters_len == special->parameters_cap) {
        size_t new_size = special->parameters_cap * PARAMETERS_MULTIPLIER * sizeof(int32_t);
        special->parameters = realloc(special->parameters, new_size);
    }

    special->parameters[special->parameters_len] = codepoint;
    special->parameters_len++;
}

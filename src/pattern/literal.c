#include "passgen/pattern/literal.h"

void passgen_pattern_literal_init(passgen_pattern_literal *literal) {
    literal->count = 0;
    literal->tainted = 0;
}

void passgen_pattern_literal_taint(passgen_pattern_literal *literal) {
    literal->tainted = 1;
}

int passgen_pattern_literal_append(
    passgen_pattern_literal *literal,
    uint32_t codepoint) {
    if(literal->count >= 7 || literal->tainted) {
        return 1;
    }

    literal->codepoints[literal->count] = codepoint;
    literal->count += 1;

    return 0;
}

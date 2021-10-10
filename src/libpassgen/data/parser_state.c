#include "passgen/data/parser_state.h"
#include "passgen/data/special.h"

void passgen_parser_state_free(struct passgen_parser_state *state) {
    switch(state->type) {
        case PASSGEN_PARSER_GROUP:
        case PASSGEN_PARSER_SET:
        case PASSGEN_PARSER_SET_RANGE:
        case PASSGEN_PARSER_REPEAT:
        case PASSGEN_PARSER_REPEAT_RANGE:
            break;
        case PASSGEN_PARSER_SPECIAL:
        case PASSGEN_PARSER_SPECIAL_NAME:
        case PASSGEN_PARSER_SPECIAL_NAME_END:
            passgen_pattern_special_free(state->data.special.special);
            break;
        case PASSGEN_PARSER_DONE:
            break;
    }
}

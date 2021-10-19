#include "passgen/data/token.h"

void passgen_token_parser_init(struct passgen_token_parser *token_parser) {
    token_parser->state = PASSGEN_TOKEN_INIT;
}

const char *passgen_token_state_to_str(enum passgen_token_state state) {
    switch(state) {
        case PASSGEN_TOKEN_INIT:
            return "Ready for parsing";
        case PASSGEN_TOKEN_ESCAPED:
            return "Parsing escaped token";
        case PASSGEN_TOKEN_UNICODE:
            return "Parsing unicode token";
        case PASSGEN_TOKEN_UNICODE_PAYLOAD:
            return "Parsing unicode token payload";
        case PASSGEN_TOKEN_ERROR_UNICODE_START:
            return "Expected '{' to come after unicode token start '\\u'";
        case PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD:
            return "";
        case PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN:
            return "";
        default:
            return NULL;
    }
}

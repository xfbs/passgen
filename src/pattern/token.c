#include "passgen/pattern/token.h"

void passgen_token_parser_init(struct passgen_token_parser *token_parser) {
    token_parser->state = PASSGEN_TOKEN_INIT;
    token_parser->offset = 0;
    token_parser->byte_offset = 0;
}

const char *passgen_token_state_string(enum passgen_token_state state) {
    switch(state) {
        case PASSGEN_TOKEN_INIT:
            return "ready for parsing";
        case PASSGEN_TOKEN_ESCAPED:
            return "parsing escaped token";
        case PASSGEN_TOKEN_UNICODE:
            return "parsing unicode token";
        case PASSGEN_TOKEN_UNICODE_PAYLOAD:
            return "parsing unicode token payload";
        case PASSGEN_TOKEN_ERROR_UNICODE_START:
            return "unexpected character while parsing unicode literal";
        case PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD:
            return "unexpected character while parsing unicode literal";
        case PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN:
            return "unicode literal payload too long";
        default:
            return NULL;
    }
}

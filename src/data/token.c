#include "passgen/data/token.h"

void passgen_token_parser_init(struct passgen_token_parser *token_parser) {
    token_parser->state = PASSGEN_TOKEN_INIT;
    token_parser->offset = 0;
    token_parser->byte_offset = 0;
}

const char *passgen_token_state_string(enum passgen_token_state state) {
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
            return "Error: unexpected character while parsing unicode literal";
        case PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD:
            return "Error: unexpected character while parsing unicode literal";
        case PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN:
            return "Error: Unicode literal payload too long";
        default:
            return NULL;
    }
}

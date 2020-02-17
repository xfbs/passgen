#include "passgen/token_new.h"
#include <stdlib.h>

static inline int token_parse_init(struct token_parser *parser, struct token *token, uint32_t codepoint) {
    if(codepoint == '\\') {
        parser->state = TOKEN_ESCAPED;
    } else {
        token->codepoint = codepoint;
        token->type = TOKEN_NORMAL;
        token->escaped = false;
        parser->state = TOKEN_INIT;
    }

    return parser->state;
}

const static int simple_escaped[][2] = {
    {'a', '\a'},
    {'b', '\b'},
    {'e', '\033'},
    {'f', '\f'},
    {'n', '\n'},
    {'r', '\r'},
    {'t', '\t'},
    {'v', '\v'},
    {'\\', '\\'},
    {0, 0}
};

static inline int token_parse_escaped(struct token_parser *parser, struct token *token, uint32_t codepoint) {
    for(size_t i = 0; simple_escaped[i][0]; i++) {
        if(codepoint == simple_escaped[i][0]) {
            token->escaped = true;
            token->type = TOKEN_NORMAL;
            token->codepoint = simple_escaped[i][1];
            parser->state = TOKEN_INIT;

            return parser->state;
        }
    }

    return parser->state;
}

int token_parse(struct token_parser *parser, struct token *token, uint32_t codepoint) {
    switch(parser->state) {
        case TOKEN_INIT:
            return token_parse_init(parser, token, codepoint);
        case TOKEN_ESCAPED:
            return token_parse_escaped(parser, token, codepoint);
        default:
            break;
    }

    return parser->state;
}

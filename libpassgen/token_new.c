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

// Simple ASCII escape map. Don't use this for large (unicode) codepoints.
// Provides efficient O(1) lookup.
const static char simple_escaped[] = {
    0,
    ['a'] = '\a',
    ['b'] = '\b',
    ['e'] = '\033',
    ['f'] = '\f',
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['\\'] = '\\'
};

static inline int token_parse_escaped(struct token_parser *parser, struct token *token, uint32_t codepoint) {
    // simple_escaped only covers ASCII, whereas codepoint could be much larger.
    if(codepoint < sizeof(simple_escaped) && simple_escaped[codepoint]) {
        token->escaped = true;
        token->normal_escaped = false;
        token->type = TOKEN_NORMAL;
        token->codepoint = simple_escaped[codepoint];
        parser->state = TOKEN_INIT;

        return parser->state;
    }

    switch(codepoint) {
        case 'u':
            parser->state = TOKEN_UNICODE;
            break;
        default:
            token->escaped = true;
            token->normal_escaped = true;
            token->codepoint = codepoint;
            parser->state = TOKEN_INIT;
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

#include "passgen/token_new.h"
#include "passgen/util.h"

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

static inline int token_parse_unicode(struct token_parser *parser, uint32_t codepoint) {
    if(codepoint == '{') {
        parser->state = TOKEN_UNICODE_PAYLOAD;
        parser->data.unicode_payload.length = 0;
        parser->data.unicode_payload.codepoint = 0;
    }

    return parser->state;
}

static inline void token_parse_unicode_payload(struct token_parser *parser, struct token *token, uint32_t codepoint) {
    // once we read the closing brace, the payload is over and we can emit the token.
    if(codepoint == '}') {
        token->codepoint = parser->data.unicode_payload.codepoint;
        token->escaped = true;
        parser->state = TOKEN_INIT;

        return;
    }

    // keep track of length, make sure it's not too long.
    parser->data.unicode_payload.length++;
    if(parser->data.unicode_payload.length > 6) {
        // error: too long
    }

    uint32_t decoded = hex_decode(codepoint);
    if(decoded < 0) {
        // error: illegal value
    }

    parser->data.unicode_payload.codepoint *= 16;
    parser->data.unicode_payload.codepoint += decoded;
}

int token_parse(struct token_parser *parser, struct token *token, uint32_t codepoint) {
    switch(parser->state) {
        case TOKEN_INIT:
            token_parse_init(parser, token, codepoint);
            break;
        case TOKEN_ESCAPED:
            token_parse_escaped(parser, token, codepoint);
            break;
        case TOKEN_UNICODE:
            token_parse_unicode(parser, codepoint);
            break;
        case TOKEN_UNICODE_PAYLOAD:
            token_parse_unicode_payload(parser, token, codepoint);
            break;
        default:
            break;
    }

    return parser->state;
}

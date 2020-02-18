/* Token Parsing functions.
 *
 * Tokens are typically individual symbols, but can also be
 * escaped symbols, or special tokens.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum token_state {
    TOKEN_INIT,
    TOKEN_ESCAPED,
    TOKEN_UNICODE,
    TOKEN_UNICODE_PAYLOAD
};

enum token_type {
    TOKEN_NORMAL,
    TOKEN_SPECIAL
};

enum token_escaped {
    TOKEN_ESCAPED_NOT,
    TOKEN_ESCAPED_SIMPLE,
    TOKEN_ESCPAED_NORMAL
};

struct token_parser {
    enum token_state state;
    union {
        // when parsing a \u{FA} unicode literal, this is where we keep the
        // state.
        struct {
            size_t length;
            uint32_t codepoint;
        } unicode_payload;
    } data;
};

struct token {
    uint32_t codepoint;
    enum token_type type;
    bool escaped;
    bool normal_escaped;
};

int token_parse(struct token_parser *parser, struct token *token, uint32_t codepoint);

#include "token_new.h"
#include "pattern/pattern.h"
#include "array.h"

enum parser_state_type {
    PARSER_OUTER,
    PARSER_SEGMENT,
    PARSER_CHAR,
    PARSER_GROUP,
    PARSER_RANGE,
    PARSER_DONE,
};

struct parser_state {
    enum parser_state_type type;
    void *data;
};

struct parser {
    passgen_array_t state;
    passgen_pattern_t parsed;
};

// parses a single token.
int parser_init(
        struct parser *parser);

int parse_token(
        struct parser *parser,
        struct token *token);

int parse_finish(
        struct parser *parser);

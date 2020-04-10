#pragma once
#include "passgen/parser.h"

int passgen_parse_start(struct parser *parser);

int passgen_parse_finish(struct parser *parser);

// parses a single token.
int passgen_parse_token(struct parser *parser, struct passgen_token *token);

int passgen_parse_token_group(
    struct parser *parser,
    struct passgen_token *token,
    struct parser_state *state);


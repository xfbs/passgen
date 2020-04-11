#pragma once
#include "passgen/data/parser.h"

int passgen_parse_start(struct parser *parser);

int passgen_parse_finish(struct parser *parser);

// parses a single token.
int passgen_parse_token(struct parser *parser, struct passgen_token *token);

int passgen_parse_group(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_set(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_set_range(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_repeat(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_repeat_range(
    struct parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

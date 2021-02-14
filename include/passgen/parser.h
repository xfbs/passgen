#pragma once
#include <stdint.h>
#include <stddef.h>

struct passgen_parser;
struct passgen_parser_state;
struct passgen_token;

int passgen_parse_finish(struct passgen_parser *parser);

int passgen_parser_utf8(
    struct passgen_parser *parser,
    uint8_t *data,
    size_t length);

int passgen_parser_unicode(
    struct passgen_parser *parser,
    uint32_t *data,
    size_t length);

// parses a single token.
int passgen_parse_token(
    struct passgen_parser *parser,
    struct passgen_token *token);

int passgen_parse_group(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_set(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_set_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_repeat(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_repeat_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_special(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_special_name(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

int passgen_parse_special_name_end(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state);

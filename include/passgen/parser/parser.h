/// @file parser.h
/// @author Patrick M. Elsen
/// @brief Passgen syntax parser.
///
/// Parsing API. Needs a `passgen_parser` struct instance, which is defined in @ref data/parser.h.
///
/// # Example
///
/// ```c
/// #include <passgen/pattern/parser.h>
/// #include <passgen/parser/parser.h>
///
/// struct passgen_parser parser = {0};
/// passgen_parser_init(&parser);
/// passgen_parser_utf8(&parser, "[a-z]{9}", 0);
/// assert(0 == passgen_parser_finish(&parser));
/// // ...
/// passgen_parser_free(&parser);
/// ```
#pragma once
#include <stdint.h>
#include <stddef.h>

struct passgen_parser;
struct passgen_parser_state;
struct passgen_token;

/// Finalize parsing.
///
/// @return 0 on success, nonzero on failure.
int passgen_parse_finish(struct passgen_parser *parser);

/// Parse a UTF-8 string.
int passgen_parser_utf8(
    struct passgen_parser *parser,
    uint8_t *data,
    size_t length);

/// Parse a sequence of unicode codepoints.
int passgen_parser_unicode(
    struct passgen_parser *parser,
    uint32_t *data,
    size_t length);

/// Parse a single token.
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

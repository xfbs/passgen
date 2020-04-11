#pragma once

#include "passgen/data/array.h"
#include "passgen/data/parser_state.h"
#include "passgen/token.h"

struct parser {
  passgen_array_t state;
  passgen_pattern_t pattern;
};

struct passgen_parser_state *
passgen_parser_get_state(struct parser *parser, size_t n);
struct passgen_parser_state *
passgen_parser_get_state_last(struct parser *parser);

struct passgen_parser_state *parser_state_push(struct parser *parser);

struct passgen_parser_state *parser_state_push_group(
    struct parser *parser,
    struct passgen_pattern_group *group,
    struct passgen_pattern_segment *segment);

struct passgen_parser_state *parser_state_push_set(
    struct parser *parser,
    struct passgen_pattern_set *set,
    struct passgen_pattern_range *range);

struct passgen_parser_state *parser_state_push_repeat(
    struct parser *parser,
    struct passgen_pattern_item *item);

void passgen_parser_init(struct parser *parser);
void passgen_parser_free(struct parser *parser);

void parser_state_pop(struct parser *parser);

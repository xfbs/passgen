#pragma once
#include <stdint.h>

#include "passgen/container/stack/parser_state.h"
#include "passgen/data/pattern.h"
#include "passgen/data/range.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/token.h"

struct passgen_parser {
  struct passgen_parser_state_stack state;
  passgen_pattern_t pattern;
};

struct passgen_parser_state *
passgen_parser_get_state(struct passgen_parser *parser, size_t n);
struct passgen_parser_state *
passgen_parser_get_state_last(struct passgen_parser *parser);

struct passgen_parser_state *
passgen_parser_state_push(struct passgen_parser *parser);

struct passgen_parser_state *passgen_parser_state_push_group(
    struct passgen_parser *parser,
    struct passgen_pattern_group *group,
    struct passgen_pattern_segment *segment);

struct passgen_parser_state *passgen_parser_state_push_set(
    struct passgen_parser *parser,
    struct passgen_pattern_set *set,
    struct passgen_pattern_range *range);

struct passgen_parser_state *passgen_parser_state_push_repeat(
    struct passgen_parser *parser,
    struct passgen_pattern_repeat *repeat);

struct passgen_parser_state *passgen_parser_state_push_special(
    struct passgen_parser *parser,
    struct passgen_pattern_special *special);

void passgen_parser_init(struct passgen_parser *parser);
void passgen_parser_free(struct passgen_parser *parser);

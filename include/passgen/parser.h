#pragma once

#include "passgen/array.h"
#include "passgen/pattern/pattern.h"
#include "passgen/token.h"

enum parser_state_type {
  PARSER_GROUP,
  PARSER_SET,
  PARSER_REPEAT,
  PARSER_DONE,
};

struct parser_state {
  enum parser_state_type type;
  union {
    struct {
      struct passgen_pattern_group *group;
      struct passgen_pattern_segment *segment;
    } group;
    struct {
      struct passgen_pattern_item *item;
    } repeat;
    struct {
      struct passgen_pattern_set *set;
      struct passgen_pattern_range *range;
    } set;
  } data;
};

struct parser {
  passgen_array_t state;
  passgen_pattern_t pattern;
};

struct parser_state *passgen_parser_get_state(struct parser *parser, size_t n);
struct parser_state *passgen_parser_get_state_last(struct parser *parser);

struct parser_state *parser_state_push(struct parser *parser);

struct parser_state *parser_state_push_group(
    struct parser *parser,
    struct passgen_pattern_group *group,
    struct passgen_pattern_segment *segment);

struct parser_state *parser_state_push_set(
    struct parser *parser,
    struct passgen_pattern_set *set,
    struct passgen_pattern_range *range);

void passgen_parser_init(struct parser *parser);
void passgen_parser_free(struct parser *parser);

void parser_state_pop(struct parser *parser);

#pragma once

#include "array.h"
#include "pattern/pattern.h"
#include "token.h"

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
    } set;
  } data;
};

struct parser {
  passgen_array_t state;
  passgen_pattern_t pattern;
};

// parses a single token.
int parser_init(struct parser *parser);

int parse_token(struct parser *parser, struct passgen_token *token);

int parse_finish(struct parser *parser);

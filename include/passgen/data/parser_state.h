#pragma once
#include "passgen/container/stack.h"
#include "passgen/data/pattern.h"

enum passgen_parser_state_type {
  PARSER_GROUP,
  PARSER_SET,
  PARSER_SET_RANGE,
  PARSER_REPEAT,
  PARSER_REPEAT_RANGE,
  PARSER_SPECIAL,
  PARSER_DONE,
};

struct passgen_parser_state {
  enum passgen_parser_state_type type;
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

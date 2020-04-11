#pragma once
#include "passgen/container/stack.h"
#include "passgen/enum_mapping.h"
#include "passgen/data/pattern.h"

enum passgen_parser_state_type {
  PASSGEN_PARSER_GROUP,
  PASSGEN_PARSER_SET,
  PASSGEN_PARSER_SET_RANGE,
  PASSGEN_PARSER_REPEAT,
  PASSGEN_PARSER_REPEAT_RANGE,
  PASSGEN_PARSER_SPECIAL,
  PASSGEN_PARSER_DONE,
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

PASSGEN_ENUM_MAPPING(passgen_parser_state_type);
PASSGEN_STACK_DECLARE(struct passgen_parser_state, passgen_parser_state);

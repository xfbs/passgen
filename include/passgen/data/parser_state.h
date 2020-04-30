#pragma once
#include "passgen/container/stack/parser_state.h"

#include "passgen/data/pattern.h"
#include "passgen/enum_mapping.h"

enum passgen_parser_state_type {
  PASSGEN_PARSER_GROUP,
  PASSGEN_PARSER_SET,
  PASSGEN_PARSER_SET_RANGE,
  PASSGEN_PARSER_REPEAT,
  PASSGEN_PARSER_REPEAT_RANGE,
  PASSGEN_PARSER_SPECIAL,
  PASSGEN_PARSER_SPECIAL_NAME,
  PASSGEN_PARSER_SPECIAL_NAME_END,
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
      struct passgen_pattern_repeat *repeat;
    } repeat;
    struct {
      struct passgen_pattern_set *set;
      struct passgen_pattern_range *range;
    } set;
    struct {
      struct passgen_pattern_special *special;
    } special;
  } data;
};

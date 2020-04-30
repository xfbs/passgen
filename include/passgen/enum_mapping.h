#pragma once
#include <stddef.h>

struct passgen_enum_mapping;

#define PASSGEN_ENUM_MAPPING(name)                          \
  extern const size_t name##_enum_count;                          \
  extern const struct passgen_enum_mapping name##_enum_by_name[]; \
  extern const struct passgen_enum_mapping name##_enum_by_value[]

struct passgen_enum_mapping {
  int value;
  const char *name;
};

struct passgen_enum_mapping *passgen_enum_by_name(
    const struct passgen_enum_mapping mapping[],
    size_t count,
    const char *name);

struct passgen_enum_mapping *passgen_enum_by_value(
    const struct passgen_enum_mapping mapping[],
    size_t count,
    int value);

PASSGEN_ENUM_MAPPING(passgen_parser_state_type);
PASSGEN_ENUM_MAPPING(passgen_pattern_kind);
PASSGEN_ENUM_MAPPING(passgen_pattern_special_kind);
PASSGEN_ENUM_MAPPING(passgen_token_state);
PASSGEN_ENUM_MAPPING(pattern_kind);
PASSGEN_ENUM_MAPPING(pattern_special_kind);
PASSGEN_ENUM_MAPPING(pattern_error_enum);
PASSGEN_ENUM_MAPPING(passgen_pronounceable_type);

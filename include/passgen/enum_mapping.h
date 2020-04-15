#pragma once
#include <stddef.h>

struct passgen_enum_mapping;

#define PASSGEN_ENUM_MAPPING(name)                          \
  extern size_t name##_enum_count;                          \
  extern struct passgen_enum_mapping name##_enum_by_name[]; \
  extern struct passgen_enum_mapping name##_enum_by_value[]

struct passgen_enum_mapping {
  int value;
  const char *name;
};

struct passgen_enum_mapping *passgen_enum_by_name(
    struct passgen_enum_mapping mapping[],
    size_t count,
    const char *name);

struct passgen_enum_mapping *passgen_enum_by_value(
    struct passgen_enum_mapping mapping[],
    size_t count,
    int value);

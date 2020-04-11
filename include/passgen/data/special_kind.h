#pragma once
#include "passgen/enum_mapping.h"

enum passgen_pattern_special_kind {
  PASSGEN_PATTERN_SPECIAL_PRONOUNCABLE,
  PASSGEN_PATTERN_SPECIAL_WORDLIST,
};

PASSGEN_ENUM_MAPPING(passgen_pattern_special_kind);

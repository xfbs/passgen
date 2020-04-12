#pragma once
#include "passgen/data/char.h"
#include "passgen/data/group.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/repeat.h"
#include "passgen/data/set.h"
#include "passgen/data/special.h"
#include "passgen/container/stack.h"

typedef struct passgen_pattern_item {
  enum passgen_pattern_kind kind;

  union {
    struct passgen_pattern_set set;
    struct passgen_pattern_group group;
    struct passgen_pattern_char character;
    struct passgen_pattern_special special;
  } data;

  struct passgen_pattern_repeat repeat;
} passgen_pattern_item_t;

PASSGEN_STACK_DECLARE(struct passgen_pattern_item, passgen_pattern_item);

void passgen_pattern_item_free(struct passgen_pattern_item *item);

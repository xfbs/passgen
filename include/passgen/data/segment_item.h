#pragma once
#include <stdbool.h>

#include "passgen/container/stack.h"
#include "passgen/data/char.h"
#include "passgen/data/group.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/repeat.h"
#include "passgen/data/set.h"
#include "passgen/data/special.h"

typedef struct passgen_pattern_item {
  enum passgen_pattern_kind kind;

  union {
    struct passgen_pattern_set set;
    struct passgen_pattern_group group;
    struct passgen_pattern_char character;
    struct passgen_pattern_special special;
  } data;

  struct passgen_pattern_repeat repeat;
  bool maybe;
} passgen_pattern_item_t;

void passgen_pattern_item_init(struct passgen_pattern_item *item);
void passgen_pattern_item_free(struct passgen_pattern_item *item);

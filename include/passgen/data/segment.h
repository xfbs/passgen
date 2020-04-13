#pragma once
#include "passgen/data/substring.h"
#include "passgen/container/stack/segment_item.h"

struct passgen_pattern_item;
struct passgen_pattern_set;
struct passgen_pattern_char;
struct passgen_pattern_group;
struct passgen_pattern_special;

typedef struct passgen_pattern_segment {
  struct passgen_pattern_substring pos;

  // array of segment_items
  struct passgen_pattern_item_stack items;

  // struct pattern_repeat repeat;
} passgen_pattern_segment_t;

void passgen_pattern_segment_init(struct passgen_pattern_segment *segment);
void passgen_pattern_segment_free(struct passgen_pattern_segment *segment);

struct passgen_pattern_item *
passgen_pattern_segment_new_item(struct passgen_pattern_segment *segment);
struct passgen_pattern_char *
passgen_pattern_segment_new_char(struct passgen_pattern_segment *segment);
struct passgen_pattern_set *
passgen_pattern_segment_new_set(struct passgen_pattern_segment *segment);
struct passgen_pattern_group *
passgen_pattern_segment_new_group(struct passgen_pattern_segment *segment);
struct passgen_pattern_special *
passgen_pattern_segment_new_special(struct passgen_pattern_segment *segment);

struct passgen_pattern_item *passgen_pattern_segment_get_item(
    struct passgen_pattern_segment *segment,
    size_t n);

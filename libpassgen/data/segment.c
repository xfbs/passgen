#include "passgen/data/segment.h"
#include "passgen/data/group.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"

void passgen_pattern_segment_init(struct passgen_pattern_segment *segment) {
  passgen_array_init(
      &segment->items,
      sizeof(struct passgen_pattern_item),
      NULL);
}

void passgen_pattern_segment_free(struct passgen_pattern_segment *segment) {
  for(size_t i = 0; i < segment->items.len; i++) {
    struct passgen_pattern_item *item = passgen_array_get(
        &segment->items,
        sizeof(struct passgen_pattern_item),
        i);

    passgen_pattern_item_free(item);
  }

  passgen_array_free(&segment->items, sizeof(struct passgen_pattern_item), NULL);
}

struct passgen_pattern_item *
passgen_pattern_segment_new_item(struct passgen_pattern_segment *segment) {
  struct passgen_pattern_item *item = passgen_array_push(
      &segment->items,
      sizeof(struct passgen_pattern_item),
      NULL);

  return item;
}

struct passgen_pattern_char *
passgen_pattern_segment_new_char(struct passgen_pattern_segment *segment) {
  struct passgen_pattern_item *item = passgen_pattern_segment_new_item(segment);
  item->kind = PASSGEN_PATTERN_CHAR;

  return &item->data.character;
}

struct passgen_pattern_set *
passgen_pattern_segment_new_set(struct passgen_pattern_segment *segment) {
  struct passgen_pattern_item *item = passgen_pattern_segment_new_item(segment);
  item->kind = PASSGEN_PATTERN_SET;
  passgen_pattern_set_init(&item->data.set);

  return &item->data.set;
}

struct passgen_pattern_group *
passgen_pattern_segment_new_group(struct passgen_pattern_segment *segment) {
  struct passgen_pattern_item *item = passgen_pattern_segment_new_item(segment);
  item->kind = PASSGEN_PATTERN_GROUP;
  passgen_pattern_group_init(&item->data.group);

  return &item->data.group;
}

struct passgen_pattern_special *
passgen_pattern_segment_new_special(struct passgen_pattern_segment *segment);

struct passgen_pattern_item *passgen_pattern_segment_get_item(
    struct passgen_pattern_segment *segment,
    size_t n) {
  return passgen_array_get(
      &segment->items,
      sizeof(struct passgen_pattern_item),
      n);
}

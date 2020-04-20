#include <stdio.h>

#include "passgen/data/array.h"
#include "passgen/data/char.h"
#include "passgen/data/group.h"
#include "passgen/data/parser.h"
#include "passgen/data/parser_state.h"
#include "passgen/data/pattern.h"
#include "passgen/data/pattern_kind.h"
#include "passgen/data/range.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"
#include "passgen/data/special.h"
#include "passgen/data/special_kind.h"
#include "passgen/data/substring.h"
#include "passgen/data/token.h"


#define SHOW_SIZE(name) printf("sizeof(" #name ") == %zu\n", sizeof(struct name))

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;

  SHOW_SIZE(passgen_parser);
  SHOW_SIZE(passgen_parser_state);

  SHOW_SIZE(passgen_pattern);
  SHOW_SIZE(passgen_pattern_group);
  SHOW_SIZE(passgen_pattern_segment);
  SHOW_SIZE(passgen_pattern_item);
  SHOW_SIZE(passgen_pattern_char);
  SHOW_SIZE(passgen_pattern_set);
  SHOW_SIZE(passgen_pattern_special);
  SHOW_SIZE(passgen_token);
  SHOW_SIZE(passgen_token_parser);

  return 0;
}

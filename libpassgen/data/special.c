#include "passgen/data/special.h"

void passgen_pattern_special_init_char(
    struct passgen_pattern_special *special,
    uint32_t kind) {
  switch(kind) {
    case 'p':
      special->kind = PASSGEN_PATTERN_SPECIAL_PRONOUNCABLE;
      break;
    case 'w':
      special->kind = PASSGEN_PATTERN_SPECIAL_WORDLIST;
      break;
    default:
      special->kind = 3;
      break;
  }
}

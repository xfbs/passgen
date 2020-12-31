#pragma once
#include "passgen/data/repeat.h"
#include "passgen/debug.h"
#include "passgen/export.h"
#include <stdint.h>

// stores a bunch of character - up to seven. why seven? this is stored in an
// union inside passgen_pattern_item, and there's enough space in there to
// store seven codepoints and a count variable, so why not.
typedef struct passgen_chars {
    uint8_t count;
    uint8_t tainted;
    int32_t codepoints[7];
} passgen_chars_t;

void passgen_pattern_char_debug(passgen_chars_t *chr, passgen_debug_t *debug);

int passgen_chars_export(passgen_chars_t *chars, void *data, passgen_export_cb *fn);

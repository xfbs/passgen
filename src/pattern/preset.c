#include "passgen/pattern/preset.h"

void passgen_preset_init(passgen_preset *preset, const char *value) {
    preset->value = value;
    preset->parsed = false;
    passgen_pattern_init(&preset->pattern);
}

int passgen_preset_parse(passgen_preset *preset) {
    return 0;
}

void passgen_preset_free(passgen_preset *preset) {
    preset->value = NULL;
    preset->parsed = false;
    passgen_pattern_free(&preset->pattern);
}

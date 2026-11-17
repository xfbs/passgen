#include "passgen/pattern/preset.h"
#include "passgen/config.h"
#include "passgen/util/clear.h"

void passgen_preset_init(passgen_preset *preset, const char *value) {
    preset->value = value;
    preset->parsed = false;
    passgen_pattern_init(&preset->pattern);
}

int passgen_preset_parse(passgen_preset *preset) {
    // TODO: implement
    (void) preset;
    return 0;
}

void passgen_preset_free(passgen_preset *preset) {
    passgen_pattern_deinit(&preset->pattern);
    PASSGEN_CLEAR(preset);
}

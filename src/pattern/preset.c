#include "passgen/pattern/preset.h"
#include "passgen/config.h"
#include "passgen/util/alloc.h"
#include "passgen/util/clear.h"

void passgen_preset_init(passgen_preset *preset, char *value) {
    preset->value = value;
    preset->parsed = false;
    preset->resolving = false;
    preset->depth = 0;
    passgen_pattern_init(&preset->pattern);
}

passgen_status
passgen_preset_parse(passgen_preset *preset, passgen_error *error) {
    if(preset->parsed) {
        return PASSGEN_OK;
    }

    // whatever was parsed up to an error is not of any use.
    passgen_status status =
        passgen_pattern_parse(&preset->pattern, error, preset->value);
    if(status != PASSGEN_OK) {
        passgen_pattern_deinit(&preset->pattern);
        passgen_pattern_init(&preset->pattern);
        return status;
    }

    preset->parsed = true;
    return PASSGEN_OK;
}

void passgen_preset_free(passgen_preset *preset) {
    passgen_pattern_deinit(&preset->pattern);
    passgen_free_string(preset->value);
    PASSGEN_CLEAR(preset);
}

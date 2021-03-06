#include <stddef.h>

#include "passgen/passgen.h"

pattern_preset pattern_presets[] = {
    {"apple1", "[a-zA-Z0-9]{3}(-[a-zA-Z0-9]{3}){3}"},
    {"apple2", "[a-zA-Z0-9]{6}(-[a-zA-Z0-9]{6}){2}"},
    {"xkcd", "(\\p[english]{6,8}-){3}\\p[english]{5,8}"},
    {NULL, NULL},
};

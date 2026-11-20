/// @file color.c
///
/// Colors in what the command-line tool prints to standard error, which are
/// only used when it is a terminal, and `NO_COLOR` is not set (see
/// https://no-color.org). Otherwise, the escape sequences for them would end
/// up in files and logs.
#include "passgen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Whether to use colors, which is decided the first time it is asked: -1
// until then.
static int passgen_cli_color_enabled = -1;

static bool passgen_cli_color(void) {
    if(passgen_cli_color_enabled < 0) {
        const char *no_color = getenv("NO_COLOR");
        passgen_cli_color_enabled =
            isatty(STDERR_FILENO) && !(no_color && no_color[0]);
    }
    return passgen_cli_color_enabled;
}

const char *passgen_cli_style(passgen_cli_style_kind style) {
    if(!passgen_cli_color()) {
        return "";
    }
    switch(style) {
        case PASSGEN_CLI_STYLE_ERROR:
            return "\033[1;31m";
        case PASSGEN_CLI_STYLE_NOTE:
            return "\033[1;33m";
        case PASSGEN_CLI_STYLE_RESET:
        default:
            return "\033[0m";
    }
}

void passgen_cli_error_print(const char *format, ...) {
    fprintf(
        stderr,
        "%serror%s: ",
        passgen_cli_style(PASSGEN_CLI_STYLE_ERROR),
        passgen_cli_style(PASSGEN_CLI_STYLE_RESET));
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

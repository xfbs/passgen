#include <passgen/error.h>
#include <stdio.h>

void passgen_error_init_raw(passgen_error *error, const char *message) {
    error->message = message;
}

void passgen_error_free(passgen_error *error) {
    (void) error;
}

void passgen_error_print(passgen_error *error, const char *pattern) {
    fprintf(
        stderr,
        "\033[1;31merror\033[0m parsing pattern: %s\n",
        error->message);
    if(error->source.file) {
        fprintf(stderr, "location %s:%zu\n", error->source.file, error->source.line);
    }
    fprintf(stderr, "\033[1;33mpattern\033[0m: %s\n", pattern);
    fprintf(stderr, "         ");
    for(size_t i = 1; i < error->offset.byte; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "\033[1;31m^\033[0m\n");
}

void passgen_error_cause_add(passgen_error *error, const passgen_error *cause) {
    (void) error;
    (void) cause;
}

void passgen_error_offset_set(passgen_error *error, size_t codepoint, size_t byte) {
    error->offset.codepoint = codepoint;
    error->offset.byte = byte;
}

void passgen_error_source_set(passgen_error *error, const char *file, size_t line) {
    error->source.file = file;
    error->source.line = line;
}

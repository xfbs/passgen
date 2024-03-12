#pragma once
#include <passgen/container/array.h>

typedef struct {
    const char *file;
    size_t line;
} passgen_error_source;

typedef struct {
    size_t codepoint;
    size_t byte;
} passgen_error_offset;

/// An error occured while parsing.
typedef struct {
    const char *message;
    // TODO: rename start
    passgen_error_offset offset;
    // TODO: rename end
    passgen_error_offset end;
    passgen_error_source source;
    passgen_array causes;
} passgen_error;

void passgen_error_init_raw(passgen_error *error, const char *message);

void passgen_error_free(passgen_error *error);

void passgen_error_print(passgen_error *error, const char *pattern);

void passgen_error_cause_add(passgen_error *error, const passgen_error *cause);

void passgen_error_source_set(passgen_error *error, const char *file, size_t line);

void passgen_error_offset_set(passgen_error *error, size_t codepoint, size_t byte);

#define passgen_error_init(error, message) \
    do { \
        passgen_error_init_raw(error, message); \
        passgen_error_source_set(error, __FILE__, __LINE__); \
    } while(false)

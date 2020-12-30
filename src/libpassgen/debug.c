#include "passgen/debug.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void struct_start(void *data, const char *name) {
    fprintf(data, "%s {", name);
}

static void struct_end(void *data, const char *name) {
    fprintf(data, "}");
}

static void member_start(void *data, const char *name) {
    fprintf(data, "%s: ", name);
}

static void member_end(void *data, const char *name) {
    fprintf(data, ", ");
}

static void value_ptr(void *data, void *ptr) {
    fprintf(data, "%p", ptr);
}

static void value_bool(void *data, bool value) {
    fprintf(data, "%s", value ? "true" : "false");
}

static void value_uint(void *data, uintmax_t value) {
    fprintf(data, "%llu", value);
}

static void value_int(void *data, intmax_t value) {
    fprintf(data, "%lli", value);
}

static void value_enum(void *data, int value, const char *name) {
    fprintf(data, "%s", name);
}

static void array_start(void *data) {
    fprintf(data, "[");
}

static void array_sep(void *data) {
    fprintf(data, ", ");
}

static void array_end(void *data) {
    fprintf(data, "]");
}

passgen_debug_t *passgen_debug_stderr() {
    passgen_debug_t *debug = malloc(sizeof(passgen_debug_t));
    assert(debug);
    memset(debug, 0, sizeof(*debug));

    debug->data = stderr;
    debug->struct_start = &struct_start;
    debug->struct_end = &struct_end;
    debug->member_start = &member_start;
    debug->member_end = &member_end;
    debug->value_pointer = &value_ptr;
    debug->value_enum = &value_enum;
    debug->value_bool = &value_bool;
    debug->value_uint = &value_uint;
    debug->value_int = &value_int;
    debug->array_start = &array_start;
    debug->array_end = &array_end;
    debug->array_sep = &array_sep;

    return debug;
}

static void pretty_indentation(int *indent) {
    if(*indent) {
        fprintf(stderr, "%*c", *indent * 4, ' ');
    }
}

static void pretty_indent(int *indent) {
    *indent += 1;
}

static void pretty_dedent(int *indent) {
    *indent -= 1;
}

static void pretty_struct_start(void *data, const char *name) {
    fprintf(stderr, "%s {\n", name);
    pretty_indent(data);
}

static void pretty_struct_end(void *data, const char *name) {
    pretty_dedent(data);
    pretty_indentation(data);
    fprintf(stderr, "}");
}

static void pretty_member_start(void *data, const char *name) {
    pretty_indentation(data);
    fprintf(stderr, "%s: ", name);
}

static void pretty_member_end(void *data, const char *name) {
    fprintf(stderr, ",\n");
}

static void pretty_value_ptr(void *data, void *ptr) {
    fprintf(stderr, "%p", ptr);
}

static void pretty_value_bool(void *data, bool value) {
    fprintf(stderr, "%s", value ? "true" : "false");
}

static void pretty_value_uint(void *data, uintmax_t value) {
    fprintf(stderr, "%llu", value);
}

static void pretty_value_int(void *data, intmax_t value) {
    fprintf(stderr, "%lli", value);
}

static void pretty_value_enum(void *data, int value, const char *name) {
    fprintf(stderr, "%s", name);
}

static void pretty_array_start(void *data) {
    fprintf(stderr, "[\n");
    pretty_indent(data);
    pretty_indentation(data);
}

static void pretty_array_sep(void *data) {
    fprintf(stderr, ",\n");
    pretty_indentation(data);
}

static void pretty_array_end(void *data) {
    fprintf(stderr, "]");
    pretty_dedent(data);
}

passgen_debug_t *passgen_debug_pretty() {
    passgen_debug_t *debug = malloc(sizeof(passgen_debug_t));
    assert(debug);
    memset(debug, 0, sizeof(*debug));

    debug->data = calloc(1, sizeof(int));
    debug->struct_start = &pretty_struct_start;
    debug->struct_end = &pretty_struct_end;
    debug->member_start = &pretty_member_start;
    debug->member_end = &pretty_member_end;
    debug->value_pointer = &pretty_value_ptr;
    debug->value_enum = &pretty_value_enum;
    debug->value_bool = &pretty_value_bool;
    debug->value_uint = &pretty_value_uint;
    debug->value_int = &pretty_value_int;
    debug->array_start = &pretty_array_start;
    debug->array_end = &pretty_array_end;
    debug->array_sep = &pretty_array_sep;

    return debug;
}

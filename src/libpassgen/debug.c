#include "passgen/debug.h"
#include <assert.h>
#include <stdio.h>

static void struct_start(void *data, const char *name) {
    fprintf(stderr, "%s {", name);
}

static void struct_end(void *data, const char *name) {
    fprintf(stderr, "}");
}

static void member_start(void *data, const char *name) {
    fprintf(stderr, "%s: ", name);
}

static void member_end(void *data, const char *name) {
    fprintf(stderr, ", ");
}

static void value_ptr(void *data, void *ptr) {
    fprintf(stderr, "%p", ptr);
}

static void array_start(void *data) {
    fprintf(stderr, "[");
}

static void array_end(void *data) {
    fprintf(stderr, "]");
}

passgen_debug_t *passgen_debug_stderr() {
    passgen_debug_t *debug = malloc(sizeof(passgen_debug_t));
    assert(debug);
    memset(debug, 0, sizeof(*debug));

    debug->struct_start = &struct_start;
    debug->struct_end = &struct_end;
    debug->member_start = &member_start;
    debug->member_end = &member_end;
    debug->value_pointer = &value_ptr;
    debug->array_start = &array_start;
    debug->array_end = &array_end;

    return debug;
}

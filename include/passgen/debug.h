#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef struct passgen_debug {
    void *data;
    void (*struct_start)(void *data, const char *name);
    void (*struct_end)(void *data, const char *name);
    void (*member_start)(void *data, const char *name);
    void (*member_end)(void *data, const char *name);
    void (*value_uint)(void *data, uint64_t);
    void (*value_int)(void *data, int64_t);
    void (*value_string)(void *data, const char *str);
    void (*value_pointer)(void *data, void *ptr);
    void (*array_start)(void *data);
    void (*array_sep)(void *data, const char *name);
    void (*array_end)(void *data);
} passgen_debug_t;

passgen_debug_t *passgen_debug_stderr();

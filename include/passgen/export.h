#pragma once
#include <stdint.h>

#define PASSGEN_EXPORT_SUCCESS 0
#define PASSGEN_EXPORT_FAILURE 1

typedef int passgen_export_cb(void *data, uint32_t codepoint);

#pragma once
#include "passgen/debug.h"
#include <stddef.h>

#ifdef PASSGEN_DEBUG

#  define assert(cond) \
    if(!(cond)) passgen_assert_fail(#cond, __FILE__, __func__, __LINE__)

#else

#  define assert(cond)

#endif

void passgen_assert_fail(
    const char *cond,
    const char *file,
    const char *func,
    size_t line);

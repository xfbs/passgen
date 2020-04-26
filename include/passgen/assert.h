#pragma once
#include "passgen/debug.h"

#ifdef PASSGEN_DEBUG
#  include <stdio.h>
#  include <stdlib.h>

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

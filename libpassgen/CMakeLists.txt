set(HEADER_LIST
    "../include/passgen/array.h"
    "../include/passgen/assert.h"
    "../include/passgen/debug.h"
    "../include/passgen/memory.h"
    "../include/passgen/memory_private.h"
    "../include/passgen/passgen.h"
    "../include/passgen/pattern.h"
    "../include/passgen/pattern_private.h")

set(SOURCE_LIST
    "array.c")

add_library(passgen ${SOURCE_LIST} ${HEADER_LIST})

target_include_directories(passgen PUBLIC ../include)
target_compile_features(passgen PUBLIC c_std_99)

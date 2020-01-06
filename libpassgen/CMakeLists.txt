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
    "${CMAKE_CURRENT_BINARY_DIR}/version.c"
    "array.c"
    "assert.c"
    "memory.c"
    "passgen.c"
    "pattern.c"
    "pattern_random.c"
    "pattern_presets.c"
    "pronounceable.c"
    "random.c"
    "reader.c"
    "pronounceable_english.c"
    "pronounceable_latin.c"
    "token.c"
    "unicode.c"
)

configure_file(version.c.in version.c)

add_library(passgen ${SOURCE_LIST} ${HEADER_LIST})

target_include_directories(passgen PUBLIC ../include)
target_compile_features(passgen PUBLIC c_std_99)
target_link_libraries(passgen utf8proc m)

if(USE_LIBEXECINFO)
target_link_libraries(passgen execinfo)
endif(USE_LIBEXECINFO)

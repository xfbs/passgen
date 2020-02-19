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
    "token_new.c"
    "token_new_json.c"
    "unicode.c"
    "utf8.c"
)

configure_file(version.c.in version.c)


add_library(passgen-objects OBJECT ${SOURCE_LIST} ${HEADER_LIST})
set_property(TARGET passgen-objects PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(passgen-objects PUBLIC ../include /usr/local/include)
target_compile_features(passgen-objects PUBLIC c_std_99)

add_library(passgen-static STATIC $<TARGET_OBJECTS:passgen-objects>)
target_link_libraries(passgen-static utf8proc m ${libjansson})
set_target_properties(passgen-static PROPERTIES OUTPUT_NAME passgen)

add_library(passgen-shared SHARED $<TARGET_OBJECTS:passgen-objects>)
target_link_libraries(passgen-shared utf8proc m ${libjansson})
set_target_properties(passgen-shared PROPERTIES OUTPUT_NAME passgen)

target_include_directories(passgen-static PUBLIC ../include /usr/local/include)
target_include_directories(passgen-shared PUBLIC ../include /usr/local/include)

#add_library(passgen ${SOURCE_LIST} ${HEADER_LIST})

#target_include_directories(passgen PUBLIC ../include /usr/local/include)
#target_compile_features(passgen PUBLIC c_std_99)
#target_link_libraries(passgen utf8proc m ${libjansson})

install(TARGETS passgen-static passgen-shared
    DESTINATION lib
    COMPONENT Libraries)

install(FILES ${HEADER_LIST}
    DESTINATION include/passgen
    COMPONENT Headers)

if(USE_LIBEXECINFO)
target_link_libraries(passgen-static execinfo)
target_link_libraries(passgen-shared execinfo)
endif(USE_LIBEXECINFO)

file(GLOB_RECURSE HEADER_LIST "../../include")

set(SOURCE_LIST
    "${CMAKE_CURRENT_BINARY_DIR}/version.c"
    "assert.c"
    "container/hashmap.c"
    "container/stack/parser_state.c"
    "container/stack/range.c"
    "container/stack/segment.c"
    "container/stack/segment_item.c"
    "data/array.c"
    "data/char.c"
    "data/group.c"
    "data/range.c"
    "data/parser.c"
    "data/pattern.c"
    "data/segment.c"
    "data/segment_item.c"
    "data/set.c"
    "data/special.c"
    "data/token.c"
    "debug.c"
    "generate.c"
    "markov.c"
    "memory.c"
    "parser.c"
    "random.c"
    "token.c"
    "utf8.c"
    "wordlist.c")

configure_file(version.c.in version.c)
configure_file(../../include/passgen/config.h.in passgen/config.h)

add_library(passgen-static ${SOURCE_LIST} ${HEADER_LIST})
target_link_libraries(passgen-static PUBLIC utf8proc m)
set_target_properties(passgen-static PROPERTIES OUTPUT_NAME passgen)
target_include_directories(passgen-static PUBLIC ../../include ${CMAKE_CURRENT_BINARY_DIR})
passgen_target_defaults(passgen-static)
passgen_check_symbols(passgen-static)

add_library(passgen-shared SHARED ${SOURCE_LIST} ${HEADER_LIST})
target_link_libraries(passgen-shared PUBLIC utf8proc m)
set_target_properties(passgen-shared PROPERTIES OUTPUT_NAME passgen)
target_include_directories(passgen-shared PUBLIC ../../include ${CMAKE_CURRENT_BINARY_DIR})
passgen_target_defaults(passgen-shared)
passgen_check_symbols(passgen-shared)

install(
  TARGETS passgen-static passgen-shared
  DESTINATION lib
  COMPONENT lib
  OPTIONAL)

if(NOT LIBPASSGEN_STATIC)
    set_target_properties(passgen-static PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

if(NOT LIBPASSGEN_SHARED)
    set_target_properties(passgen-shared PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

install(
  DIRECTORY ../../include/passgen
  DESTINATION include
  COMPONENT dev)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/passgen/config.h"
    DESTINATION include/passgen
    COMPONENT dev)

if(USE_LIBEXECINFO)
  target_link_libraries(passgen-static PRIVATE execinfo)
  #target_link_libraries(passgen-shared PRIVATE execinfo)
endif(USE_LIBEXECINFO)

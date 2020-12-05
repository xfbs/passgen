set(HEADER_LIST
    "../../include/passgen/assert.h"
    "../../include/passgen/container/stack.h"
    "../../include/passgen/container/stack_impl.h"
    "../../include/passgen/data/array.h"
    "../../include/passgen/data/char.h"
    "../../include/passgen/data/group.h"
    "../../include/passgen/data/parser.h"
    "../../include/passgen/data/parser_state.h"
    "../../include/passgen/data/pattern.h"
    "../../include/passgen/data/pattern_kind.h"
    "../../include/passgen/data/range.h"
    "../../include/passgen/data/repeat.h"
    "../../include/passgen/data/segment.h"
    "../../include/passgen/data/segment_item.h"
    "../../include/passgen/data/set.h"
    "../../include/passgen/data/special.h"
    "../../include/passgen/data/special_kind.h"
    "../../include/passgen/data/substring.h"
    "../../include/passgen/data/token.h"
    "../../include/passgen/debug.h"
    "../../include/passgen/memory.h"
    "../../include/passgen/memory_private.h"
    "../../include/passgen/parser.h"
    "../../include/passgen/markov.h"
    "../../include/passgen/random.h"
    "../../include/passgen/token.h"
    "../../include/passgen/utf8.h"
    "../../include/passgen/util.h"
    "../../include/passgen/version.h"
    "../../include/passgen/wordlist.h")

set(SOURCE_LIST
    "${CMAKE_CURRENT_BINARY_DIR}/version.c"
    "assert.c"
    "container/stack/parser_state.c"
    "container/stack/range.c"
    "container/stack/segment.c"
    "container/stack/segment_item.c"
    "data/array.c"
    "data/group.c"
    "data/parser.c"
    "data/pattern.c"
    "data/segment.c"
    "data/segment_item.c"
    "data/set.c"
    "data/special.c"
    "data/token.c"
    "generate.c"
    "markov.c"
    "memory.c"
    "parser.c"
    "random.c"
    "token.c"
    "utf8.c"
    "wordlist.c")

configure_file(version.c.in version.c)

#add_library(passgen-objects OBJECT ${SOURCE_LIST} ${HEADER_LIST})
#target_include_directories(
#  passgen-objects PUBLIC $<TARGET_PROPERTY:utf8proc,INCLUDE_DIRECTORIES>
#  $<TARGET_PROPERTY:jansson,INTERFACE_INCLUDE_DIRECTORIES>)
# target_link_libraries(passgen-objects utf8proc m ${libjansson})
#set_property(TARGET passgen-objects PROPERTY POSITION_INDEPENDENT_CODE 1)

#target_include_directories(passgen-objects PUBLIC ../include)
#target_compile_features(passgen-objects PUBLIC c_std_99)


add_library(passgen-static ${SOURCE_LIST} ${HEADER_LIST})
target_link_libraries(passgen-static PUBLIC utf8proc m jansson::jansson)
set_target_properties(passgen-static PROPERTIES OUTPUT_NAME passgen)
target_include_directories(passgen-static PUBLIC ../../include)
passgen_target_defaults(passgen-static)
passgen_check_symbols(passgen-static)

if(NOT BUILD_STATIC)
  add_library(passgen-shared SHARED ${SOURCE_LIST} ${HEADER_LIST})
  target_link_libraries(passgen-shared PUBLIC utf8proc m jansson::jansson)
  set_target_properties(passgen-shared PROPERTIES OUTPUT_NAME passgen)
  target_include_directories(passgen-shared PUBLIC ../../include)
  passgen_target_defaults(passgen-shared)
endif()

# add_library(passgen ${SOURCE_LIST} ${HEADER_LIST})

# target_include_directories(passgen PUBLIC ../include /usr/local/include)
# target_compile_features(passgen PUBLIC c_std_99) target_link_libraries(passgen
# utf8proc m ${libjansson})

install(
  TARGETS passgen-static #passgen-shared
  DESTINATION lib
  COMPONENT Libraries)

install(
  DIRECTORY ../../include/passgen
  DESTINATION include
  COMPONENT Headers)

if(USE_LIBEXECINFO)
  target_link_libraries(passgen-static PRIVATE execinfo)
  #target_link_libraries(passgen-shared PRIVATE execinfo)
endif(USE_LIBEXECINFO)

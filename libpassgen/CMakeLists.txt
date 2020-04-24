set(HEADER_LIST
    "../include/passgen/assert.h"
    "../include/passgen/container/stack.h"
    "../include/passgen/container/stack_impl.h"
    "../include/passgen/data/array.h"
    "../include/passgen/data/char.h"
    "../include/passgen/data/group.h"
    "../include/passgen/data/parser.h"
    "../include/passgen/data/parser_state.h"
    "../include/passgen/data/pattern.h"
    "../include/passgen/data/pattern_kind.h"
    "../include/passgen/data/range.h"
    "../include/passgen/data/repeat.h"
    "../include/passgen/data/segment.h"
    "../include/passgen/data/segment_item.h"
    "../include/passgen/data/set.h"
    "../include/passgen/data/special.h"
    "../include/passgen/data/special_kind.h"
    "../include/passgen/data/substring.h"
    "../include/passgen/data/token.h"
    "../include/passgen/debug.h"
    "../include/passgen/enum_mapping.h"
    "../include/passgen/memory.h"
    "../include/passgen/memory_private.h"
    "../include/passgen/parser.h"
    "../include/passgen/passgen.h"
    "../include/passgen/passgen2.h"
    "../include/passgen/pattern.h"
    "../include/passgen/pattern_private.h"
    "../include/passgen/pronounceable.h"
    "../include/passgen/pronounceable_private.h"
    "../include/passgen/random.h"
    "../include/passgen/reader.h"
    "../include/passgen/token.h"
    "../include/passgen/token_old.h"
    "../include/passgen/unicode.h"
    "../include/passgen/utf8.h"
    "../include/passgen/util.h"
    "../include/passgen/version.h"
    "../include/passgen/wordlist.h")

set(SOURCE_LIST
    "${CMAKE_CURRENT_BINARY_DIR}/version.c"
    "${CMAKE_CURRENT_BINARY_DIR}/enum_mapping_autogen.c"
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
    "enum_mapping.c"
    "generate.c"
    "json/token.c"
    "memory.c"
    "parser.c"
    "passgen.c"
    "passgen2.c"
    "pattern.c"
    "pattern_presets.c"
    "pattern_random.c"
    "pronounceable.c"
    "${CMAKE_CURRENT_BINARY_DIR}/pronounceable_latin_autogen.c"
    "${CMAKE_CURRENT_BINARY_DIR}/pronounceable_german_autogen.c"
    "${CMAKE_CURRENT_BINARY_DIR}/pronounceable_english_autogen.c"
    "random.c"
    "reader.c"
    "token.c"
    "token_old.c"
    "unicode.c"
    "utf8.c")

configure_file(version.c.in version.c)

# this automatically generates enum mappings (mappings from enum integer to
# string and back) from the header files.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/enum_mapping_autogen.c
  COMMAND ${PROJECT_SOURCE_DIR}/scripts/generate_enum_mapping.rb
          ${CMAKE_CURRENT_BINARY_DIR}/enum_mapping_autogen.c ${HEADER_LIST}
  DEPENDS ${PROJECT_SOURCE_DIR}/scripts/generate_enum_mapping.rb ${HEADER_LIST}
  COMMENT "Generating enum mapping"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM)

# for the pronounceable word generation to work, we take a word list (they
# are compressed in data/name.txt.gz), and run it through this script, which
# generates a markov chain data structure for us, which is then compiled into
# the binary.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pronounceable_latin_autogen.c
  COMMAND
    ${PROJECT_SOURCE_DIR}/scripts/generate_markov.rb
    ${PROJECT_SOURCE_DIR}/data/latin.txt.gz passgen_pronounceable_latin >
    ${CMAKE_CURRENT_BINARY_DIR}/pronounceable_latin_autogen.c
  DEPENDS ${PROJECT_SOURCE_DIR}/scripts/generate_markov.rb
          ${PROJECT_SOURCE_DIR}/data/latin.txt.gz
  COMMENT "Generating markov chain for latin words"
  VERBATIM)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pronounceable_german_autogen.c
  COMMAND
    ${PROJECT_SOURCE_DIR}/scripts/generate_markov.rb
    ${PROJECT_SOURCE_DIR}/data/german.txt.gz passgen_pronounceable_german >
    ${CMAKE_CURRENT_BINARY_DIR}/pronounceable_german_autogen.c
  DEPENDS ${PROJECT_SOURCE_DIR}/scripts/generate_markov.rb
          ${PROJECT_SOURCE_DIR}/data/german.txt.gz
  COMMENT "Generating markov chain for latin words"
  VERBATIM)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pronounceable_english_autogen.c
  COMMAND
    ${PROJECT_SOURCE_DIR}/scripts/generate_markov.rb
    ${PROJECT_SOURCE_DIR}/data/english.txt.gz passgen_pronounceable_english >
    ${CMAKE_CURRENT_BINARY_DIR}/pronounceable_english_autogen.c
  DEPENDS ${PROJECT_SOURCE_DIR}/scripts/generate_markov.rb
          ${PROJECT_SOURCE_DIR}/data/english.txt.gz
  COMMENT "Generating markov chain for latin words"
  VERBATIM)

add_library(passgen-objects OBJECT ${SOURCE_LIST} ${HEADER_LIST})
target_include_directories(
  passgen-objects PUBLIC $<TARGET_PROPERTY:utf8proc,INCLUDE_DIRECTORIES>)
# target_link_libraries(passgen-objects utf8proc m ${libjansson})
set_property(TARGET passgen-objects PROPERTY POSITION_INDEPENDENT_CODE 1)
passgen_target_defaults(passgen-objects)

target_include_directories(passgen-objects PUBLIC ../include /usr/local/include)
target_compile_features(passgen-objects PUBLIC c_std_99)

add_library(passgen-static STATIC $<TARGET_OBJECTS:passgen-objects>)
target_link_libraries(passgen-static INTERFACE utf8proc m ${libjansson})
set_target_properties(passgen-static PROPERTIES OUTPUT_NAME passgen)
passgen_check_symbols(passgen-static)

add_library(passgen-shared SHARED $<TARGET_OBJECTS:passgen-objects>)
target_link_libraries(passgen-shared PRIVATE utf8proc m ${libjansson})
set_target_properties(passgen-shared PROPERTIES OUTPUT_NAME passgen)

target_include_directories(passgen-static PUBLIC ../include /usr/local/include)
target_include_directories(passgen-shared PUBLIC ../include /usr/local/include)

# add_library(passgen ${SOURCE_LIST} ${HEADER_LIST})

# target_include_directories(passgen PUBLIC ../include /usr/local/include)
# target_compile_features(passgen PUBLIC c_std_99) target_link_libraries(passgen
# utf8proc m ${libjansson})

install(
  TARGETS passgen-static passgen-shared
  DESTINATION lib
  COMPONENT Libraries)

install(
  FILES ${HEADER_LIST}
  DESTINATION include/passgen
  COMPONENT Headers)

if(USE_LIBEXECINFO)
  target_link_libraries(passgen-static execinfo)
  target_link_libraries(passgen-shared execinfo)
endif(USE_LIBEXECINFO)

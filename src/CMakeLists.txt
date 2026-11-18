file(GLOB_RECURSE HEADER_LIST "../../include")

set(SOURCE_LIST
    "${CMAKE_CURRENT_BINARY_DIR}/version.c"
    "alloc.c"
    "assert.c"
    "container/hashmap.c"
    "container/stack.c"
    "env.c"
    "error.c"
    "generate.c"
    "markov.c"
    $<$<BOOL:${PASSGEN_MONOCYPHER}>:monocypher.c>
    "parser/parser.c"
    "parser/token.c"
    "passgen.c"
    "pattern/group.c"
    "pattern/literal.c"
    "pattern/pattern.c"
    "pattern/preset.c"
    "pattern/segment.c"
    "pattern/segment_item.c"
    "pattern/set.c"
    "pattern/special.c"
    $<$<BOOL:${PASSGEN_MONOCYPHER}>:random/deterministic.c>
    "random/random.c"
    "random/system.c"
    "random/xorshift.c"
    "random/zero.c"
    "util/siphash.c"
    "util/utf8.c"
    "wordlist.c"
)

configure_file(version.c.in version.c)
configure_file(../include/passgen/config.h.in passgen/config.h)

add_library(passgen-static ${SOURCE_LIST} ${HEADER_LIST})
target_link_libraries(passgen-static PUBLIC m)
set_target_properties(passgen-static PROPERTIES OUTPUT_NAME passgen)
target_include_directories(passgen-static PUBLIC ../include ${CMAKE_CURRENT_BINARY_DIR})
passgen_target_defaults(passgen-static)
passgen_check_symbols(passgen-static)

add_library(passgen-shared SHARED ${SOURCE_LIST} ${HEADER_LIST})
target_link_libraries(passgen-shared PRIVATE m)
set_target_properties(passgen-shared PROPERTIES OUTPUT_NAME passgen)

# version the shared library, so that programs linked against it don't end up
# using an incompatible one. before version 1.0.0, minor versions may be
# incompatible with each other, so the minor version is part of the name
# which programs are linked against (libpassgen.so.0.2). from 1.0.0 on, this
# should only be the major version.
if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(PASSGEN_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
else()
    set(PASSGEN_SOVERSION "${PROJECT_VERSION_MAJOR}")
endif()
set_target_properties(
    passgen-shared PROPERTIES VERSION ${PROJECT_VERSION}
                              SOVERSION ${PASSGEN_SOVERSION})
target_include_directories(passgen-shared PUBLIC ../include ${CMAKE_CURRENT_BINARY_DIR})
passgen_target_defaults(passgen-shared)
passgen_check_symbols(passgen-shared)
passgen_metrics(passgen-shared)

include(GenerateExportHeader)
# functions and variables which are part of the public interface are marked
# with PASSGEN_EXPORT, which comes from this generated header. it is part of
# the public interface itself, and installed along with the other headers.
generate_export_header(passgen-shared
    BASE_NAME passgen
    EXPORT_FILE_NAME passgen/export.h)
target_compile_definitions(passgen-static PUBLIC -DPASSGEN_STATIC_DEFINE)

# only what is marked that way is visible from outside of the shared library.
# everything else is internal, and can only be used when linking against the
# static library, which is what the tests do.
set_target_properties(passgen-shared PROPERTIES C_VISIBILITY_PRESET hidden)

# the shared library is needed to run the command-line tool when it is linked
# against it, so it is part of the runtime. the static library, and the link
# to the shared one which the linker uses, are only needed to build against
# the library.
# only the libraries which are built are installed.
set(passgen_installed_libraries)
if(LIBPASSGEN_STATIC)
    list(APPEND passgen_installed_libraries passgen-static)
endif()
if(LIBPASSGEN_SHARED)
    list(APPEND passgen_installed_libraries passgen-shared)
endif()
if(passgen_installed_libraries)
    install(
        TARGETS ${passgen_installed_libraries}
        ARCHIVE DESTINATION ${PASSGEN_INSTALL_LIBDIR} COMPONENT development
        LIBRARY DESTINATION ${PASSGEN_INSTALL_LIBDIR} COMPONENT runtime
            NAMELINK_COMPONENT development
        RUNTIME DESTINATION bin COMPONENT runtime)
endif()

if(NOT LIBPASSGEN_STATIC)
    set_target_properties(passgen-static PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

if(NOT LIBPASSGEN_SHARED)
    set_target_properties(passgen-shared PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# only the public headers are installed. the templates for generated headers
# are not headers themselves.
install(
    DIRECTORY ../include/passgen
    DESTINATION include
    COMPONENT development
    PATTERN "*.in" EXCLUDE)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/passgen/config.h"
          "${CMAKE_CURRENT_BINARY_DIR}/passgen/export.h"
    DESTINATION include/passgen
    COMPONENT development)

# for building against the library with pkg-config, such as with
# `pkg-config --cflags --libs passgen`.
configure_file(passgen.pc.in passgen.pc @ONLY)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/passgen.pc"
    DESTINATION ${PASSGEN_INSTALL_LIBDIR}/pkgconfig
    COMPONENT development)

if(USE_LIBEXECINFO)
  target_link_libraries(passgen-static PRIVATE execinfo)
  #target_link_libraries(passgen-shared PRIVATE execinfo)
endif(USE_LIBEXECINFO)

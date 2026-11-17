cmake_minimum_required(VERSION 3.16)

project(
    passgen
    VERSION 0.2.0
    DESCRIPTION "Regex-based password generator"
    LANGUAGES C)

# project build options.
option(BUILD_TOOLS "Build utility binaries." YES)
option(BUILD_BENCH "Build benchmark utilities." YES)
option(BUILD_GIT_INFO "Bake git commit information into the binaries." ON)
option(BUILD_TESTING "Build unit tests" ON)
option(BUILD_DATA "Build data" ON)

option(LIBPASSGEN_STATIC "Build Passgen static library (libpassgen.a)" ON)
option(LIBPASSGEN_SHARED "Build Passgen dynamic library (libpassgen.so)" ON)
option(PASSGEN_STATIC "Link Passgen CLI statically" OFF)

option(USE_BACKTRACE "Enable backtrace() for debug builds (not always available)." ON)
option(USE_LIBEXECINFO
       "Use libexecinfo. For systems that don't have backtrace in libc." OFF)

# TODO: disable when not building for linux
option(PASSGEN_MLOCK "Use mlock() to lock memory on startup, preventing leaks through swapping" ON)

option(PASSGEN_MEMORY "Build with option to override memory management" ON)
option(PASSGEN_SECCOMP "Build with seccomp filters" OFF)

option(PASSGEN_MONOCYPHER "Build with monocypher" ON)
option(PASSGEN_FUZZ "Build fuzzing harnesses with libFuzzer (needs Clang)" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(PASSGEN_DEBUG "Include debugging features for passgen" ON)
else()
    option(PASSGEN_DEBUG "Include debugging features for passgen" OFF)
endif()

# against what do we link tools?
if(PASSGEN_STATIC OR NOT LIBPASSGEN_SHARED)
    set(PASSGEN_LIB passgen-static)
else()
    set(PASSGEN_LIB passgen-shared)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(FindPkgConfig)
include(ClangFormat)
include(CMakeFormat)
include(Passgen)
include(Sanitizers)
include(CodeCoverage)
include(IWYU)

clangformat_setup()
cmakeformat_setup(
    CMakeLists.txt
    data/CMakeLists.txt
    src/CMakeLists.txt
    src/bench/CMakeLists.txt
    src/cli/CMakeLists.txt
    src/tests/CMakeLists.txt
    src/tools/CMakeLists.txt)
passgen_check_symbols_setup()
iwyu_setup()
passgen_metrics_setup()

# information about the build, which is shown along with the version. this is
# what git describes the commit as, unless it is set to something else (which
# is useful when building from something that is not a git repository).
set(PASSGEN_BUILD_INFO
    ""
    CACHE STRING "Information about the build to show along with the version.")
if(NOT PASSGEN_BUILD_INFO AND BUILD_GIT_INFO)
    include(GitInfo)
    git_info()
    set(PASSGEN_BUILD_INFO "${PROJECT_GIT_DESCRIBE}")
endif()

# dependency (only on alpine): libexecinfo-dev
if(USE_LIBEXECINFO)
    find_library(libexecinfo execinfo)
endif(USE_LIBEXECINFO)

# dependency: secconf, if requested.
if(PASSGEN_SECCOMP)
    if(PASSGEN_STATIC)
        find_library(libseccomp NAMES libseccomp.a seccomp)
    else()
        find_library(libseccomp seccomp)
    endif()
endif()

# for fuzzing, everything needs to be instrumented, so that the fuzzing engine
# can tell which code an input reaches.
if(PASSGEN_FUZZ)
    add_compile_options(-fsanitize=fuzzer-no-link)
endif()

# build libpassgen
add_subdirectory(src)

if(BUILD_DATA)
    add_subdirectory(data)
endif()

if(BUILD_BENCH)
    add_subdirectory(src/bench)
endif()

# build passgen binary and perhaps utils
add_subdirectory(src/cli)

if(BUILD_TOOLS)
    add_subdirectory(src/tools)
endif()

# unit tests and CLI tests, run with `ctest` or by building the `check` target.
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(src/tests)

    add_test(NAME passgen-test COMMAND passgen-test)
    add_test(NAME passgen-cli COMMAND ${CMAKE_SOURCE_DIR}/scripts/test-passgen-cli.sh
                                      $<TARGET_FILE:passgen-cli>)

    # fuzzing harnesses, which are run over their corpus as tests. these use
    # POSIX functions which are not there on Windows.
    if(NOT WIN32)
        add_subdirectory(src/fuzz)
    endif()

    # unlike the builtin `test` target, this makes sure everything is built.
    add_custom_target(check DEPENDS test-lib test-cli)
endif()

# setup and install man page
configure_file(man/passgen.1.in passgen.1)
install(
    FILES ${PROJECT_BINARY_DIR}/passgen.1
    DESTINATION share/man/man1
    COMPONENT runtime)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generates random sequences from a regex-like pattern")
set(CPACK_PACKAGE_VENDOR "xfbs")
# this ends up in the package metadata (for example `apt show passgen`), so it
# has to be plain text.
set(CPACK_PACKAGE_DESCRIPTION
"Passgen generates random sequences from a pattern written in a dialect of
regular expressions. It is built for generating secure passphrases: it can
pick words from wordlists, make up pronounceable words using a Markov chain,
and measure the entropy of every sequence it generates.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://passgen.io")
set(CPACK_PACKAGE_CONTACT "Patrick Elsen <pelsen@xfbs.net>")
set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")

# all targets are compiled with debug information, strip it from the binaries
# and libraries that end up in packages.
set(CPACK_STRIP_FILES ON)

set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEB_COMPONENT_INSTALL OFF)

if(PASSGEN_SECCOMP)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libseccomp2 (>= 2.5.1)")
else()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6")
endif()

# development package
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS "runtime")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_NAME "passgen-dev")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_ARCHITECTURE "all")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_DEPENDS "libc6-dev | libc-dev")

# data package
set(CPACK_COMPONENT_DATA_DEPENDS "runtime")
set(CPACK_DEBIAN_DATA_PACKAGE_NAME "passgen-data")
set(CPACK_DEBIAN_DATA_PACKAGE_ARCHITECTURE "all")
set(CPACK_DEBIAN_DATA_PACKAGE_DEPENDS "")

# runtime package
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "passgen")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SECTION "utils")
if(PASSGEN_SECCOMP)
    set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS "libc6, libseccomp2 (>= 2.5.1)")
else()
    set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS "libc6")
endif()


include(CPack)


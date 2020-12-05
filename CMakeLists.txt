cmake_minimum_required(VERSION 3.10)

project(
  passgen
  VERSION 1.0.0
  DESCRIPTION "Regex-based password generator"
  LANGUAGES C)

# project build options.
option(BUILD_UTF8PROC "Build utf8proc, a dependency." YES)
option(BUILD_JANSSON "Build jansson, a dependecy." OFF)
option(BUILD_TOOLS "Build utility binaries." YES)
option(BUILD_BENCH "Build benchmark utilities." YES)
option(BUILD_GIT_INFO "Bake git commit information into the binaries." ON)
option(BUILD_STATIC "Built targets statically." OFF)

option(USE_BACKTRACE "Use backtrace() for debug builds (not always available)." ON)
option(USE_LIBEXECINFO
       "Use libexecinfo. For systems that don't have backtrace in libc." OFF)

option(PASSGEN_CLDR "Include code for parsing the CLDR" OFF)

if(PASSGEN_CLDR)
    include(FindEXPAT)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#include(ExternalProject)
include(FindPkgConfig)
include(ClangFormat)
include(CMakeFormat)
include(CTest)
include(Passgen)
include(Sanitizers)
include(CodeCoverage)
include(IWYU)

clangformat_setup()
cmakeformat_setup(CMakeLists.txt libpassgen/CMakeLists.txt tests/CMakeLists.txt
                  tools/CMakeLists.txt bin/CMakeLists.txt)
passgen_check_symbols_setup()
iwyu_setup()

# get commit info, if git is available.
if(BUILD_GIT_INFO)
  include(GitInfo)
  git_info()
endif()

add_subdirectory(extern)

if(BUILD_BENCH)
  add_subdirectory(src/bench)
endif()

# build libpassgen
add_subdirectory(src/libpassgen)

# build passgen binary and perhaps utils
add_subdirectory(src/passgen)

if(BUILD_TOOLS)
  add_subdirectory(src/tools)
endif()

# unit tests
if(BUILD_TESTING)
  add_subdirectory(src/tests)
endif()

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Passgen")
set(CPACK_PACKAGE_VENDOR "xfbs")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "3")
set(CPACK_PACKAGE_VERSION_PATCH "2")
include(CPack)

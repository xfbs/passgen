cmake_minimum_required(VERSION 3.10)

project(
  passgen
  VERSION 1.0.0
  DESCRIPTION "Regex-based password generator"
  LANGUAGES C)

# project build options.
option(BUILD_UTF8PROC "Build utf8proc, a dependency." YES)
option(BUILD_UTILS "Build utility binaries." OFF)
option(BUILD_FUZZER
       "Build fuzzer using afl (requires setting compiler to afl-gcc)." OFF)
option(BUILD_GIT_INFO "Bake git commit information into the binaries." ON)
option(BUILD_STATIC "Built targets statically." OFF)

option(USE_LIBEXECINFO
       "Use libexecinfo. For systems that don't have backtrace in libc." OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(ExternalProject)
include(ClangFormat)
include(CMakeFormat)
include(CTest)
include(Passgen)
include(Sanitizers)
include(CodeCoverage)

clangformat_setup()
cmakeformat_setup(CMakeLists.txt libpassgen/CMakeLists.txt tests/CMakeLists.txt
                  tools/CMakeLists.txt bin/CMakeLists.txt)

# get commit info, if git is available.
if(BUILD_GIT_INFO)
  include(GitInfo)
  git_info()
endif()

add_subdirectory(bench)

# dependency: utf8proc
if(BUILD_UTF8PROC)
  add_subdirectory(extern/utf8proc)
else()
  find_package(utf8proc)
endif()

# dependency (only on alpine): libexecinfo-dev
if(USE_LIBEXECINFO)
  find_library(libexecinfo execinfo)
endif(USE_LIBEXECINFO)

find_library(libjansson jansson)

# build libpassgen
add_subdirectory(libpassgen)

# build passgen binary and perhaps utils
add_subdirectory(bin)
add_subdirectory(tools)

# unit tests
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

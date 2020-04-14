cmake_minimum_required(VERSION 3.10)

project(
  passgen
  VERSION 1.0.0
  DESCRIPTION "Regex-based password generator"
  LANGUAGES C)

# project build options.
option(BUILD_UTF8PROC "Build utf8proc, a dependency." YES)
option(
  BUILD_LIBPASSGEN
  "Build libpassgen. Enabled by default, if disabled uses system-wide libpassgen."
  ON)
option(BUILD_PASSGEN "Build passgen main binary." ON)
option(BUILD_CMOCKA "Build cmocka library for unit tests." YES)
option(BUILD_UTILS "Build utility binaries." OFF)
option(BUILD_BINARY "Build passgen binary." ON)
option(BUILD_UTILS "Build utility executables." OFF)
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
if(BUILD_LIBPASSGEN)
  add_subdirectory(libpassgen)
endif()

# build passgen binary and perhaps utils
add_subdirectory(bin)
add_subdirectory(tools)

# unit tests
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

cmake_minimum_required(VERSION 3.10)

project(
  passgen
  VERSION 1.0.0
  DESCRIPTION "Regex-based password generator"
  LANGUAGES C)

# project build options.
option(BUILD_UTF8PROC "Build utf8proc, a dependency." YES)
option(BUILD_TOOLS "Build utility binaries." YES)
option(BUILD_BENCH "Build benchmark utilities." YES)
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

# dependency: utf8proc
if(BUILD_UTF8PROC)
  add_subdirectory(extern/utf8proc)
  # target_code_coverage(utf8proc)
else()
  find_package(utf8proc)
endif()

# dependency (only on alpine): libexecinfo-dev
if(USE_LIBEXECINFO)
  find_library(libexecinfo execinfo)
endif(USE_LIBEXECINFO)

find_library(libjansson jansson)

if(BUILD_BENCH)
  add_subdirectory(bench)
endif()

# build libpassgen
add_subdirectory(libpassgen)

# build passgen binary and perhaps utils
add_subdirectory(bin)

if(BUILD_TOOLS)
  add_subdirectory(tools)
endif()

# unit tests
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Passgen")
set(CPACK_PACKAGE_VENDOR "xfbs")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "3")
set(CPACK_PACKAGE_VERSION_PATCH "2")
include(CPack)

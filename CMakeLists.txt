cmake_minimum_required(VERSION 3.10)

project(
    passgen
    VERSION 1.0.0
    DESCRIPTION "Regex-based password generator"
    LANGUAGES C)

# project build options.
option(BUILD_TOOLS "Build utility binaries." YES)
option(BUILD_BENCH "Build benchmark utilities." YES)
option(BUILD_GIT_INFO "Bake git commit information into the binaries." ON)
option(BUILD_TESTING "Build unit tests" ON)
option(BUILD_DATA "Build data" ON)

option(LIBPASSGEN_STATIC "Build libpassgen.a" ON)
option(LIBPASSGEN_SHARED "Build libpassgen.so" ON)
option(PASSGEN_STATIC "Link passgen statically" OFF)

option(USE_BACKTRACE "Use backtrace() for debug builds (not always available)." ON)
option(USE_LIBEXECINFO
       "Use libexecinfo. For systems that don't have backtrace in libc." OFF)

# TODO: disable when not building for linux
option(PASSGEN_MLOCK "Use mlock() to lock memory on startup, preventing leaks through swapping" ON)

option(PASSGEN_MEMORY "Build with option to override memory management" ON)
option(PASSGEN_SECCOMP "Build with seccomp filters" OFF)

option(PASSGEN_MONOCYPHER "Build with monocypher" ON)

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
cmakeformat_setup(CMakeLists.txt libpassgen/CMakeLists.txt tests/CMakeLists.txt
                  tools/CMakeLists.txt bin/CMakeLists.txt)
passgen_check_symbols_setup()
iwyu_setup()
passgen_metrics_setup()

# get commit info, if git is available.
if(BUILD_GIT_INFO)
    include(GitInfo)
    git_info()
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

# unit tests
if(BUILD_TESTING)
    add_subdirectory(src/tests)
endif()

# setup and install man page
configure_file(man/passgen.1.in passgen.1)
install(
    FILES ${PROJECT_BINARY_DIR}/passgen.1
    DESTINATION share/man/man1
    COMPONENT runtime)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Passgen")
set(CPACK_PACKAGE_VENDOR "xfbs")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_CONTACT "Patrick Elsen <pelsen@xfbs.net>")
set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
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


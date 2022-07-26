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
option(BUILD_TESTING "Build unit tests" ON)

option(LIBPASSGEN_STATIC "Build libpassgen.a" ON)
option(LIBPASSGEN_SHARED "Build libpassgen.so" ON)
option(PASSGEN_STATIC "Link passgen statically" OFF)

option(USE_BACKTRACE "Use backtrace() for debug builds (not always available)." ON)
option(USE_LIBEXECINFO
       "Use libexecinfo. For systems that don't have backtrace in libc." OFF)

option(PASSGEN_CLDR "Include code for parsing the CLDR" OFF)
option(PASSGEN_MEMORY "Build with option to override memory management" ON)
option(PASSGEN_SECCOMP "Build with seccomp filters" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(PASSGEN_DEBUG "Include debugging features for passgen" ON)
else()
    option(PASSGEN_DEBUG "Include debugging features for passgen" OFF)
endif()

if(PASSGEN_CLDR)
    include(FindEXPAT)
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

add_subdirectory(extern)

# build libpassgen
add_subdirectory(src)

if(BUILD_BENCH)
    add_subdirectory(src/bench)
endif()

# build passgen binary and perhaps utils
add_subdirectory(src/passgen)

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
    COMPONENT man)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Passgen")
set(CPACK_PACKAGE_VENDOR "xfbs")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
if(PASSGEN_SECCOMP)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libseccomp2 (>= 2.5.1)")
else()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
endif()
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_PACKAGE_CONTACT "Patrick Elsen <patrick@rtbrick.com>")
set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})

include(CPack)

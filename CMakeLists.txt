cmake_minimum_required(VERSION 3.16)

project(
    passgen
    VERSION 0.4.0
    DESCRIPTION "Regex-based password generator"
    LANGUAGES C)

# project build options.
option(BUILD_TOOLS "Build utility binaries." YES)
option(BUILD_BENCH "Build benchmark utilities." YES)
option(BUILD_GIT_INFO "Bake git commit information into the binaries." ON)
option(BUILD_TESTING "Build unit tests" ON)
option(BUILD_DATA "Build data" ON)
option(PASSGEN_TEST_CXX
       "Check that the public headers work from C++ (needs a C++ compiler, which should be the counterpart of the C compiler)" OFF)

option(LIBPASSGEN_STATIC "Build Passgen static library (libpassgen.a)" ON)
option(LIBPASSGEN_SHARED "Build Passgen dynamic library (libpassgen.so)" ON)
option(PASSGEN_STATIC "Link Passgen CLI fully statically, including the C library" OFF)
option(PASSGEN_CLI_SHARED "Link Passgen CLI against the shared library (libpassgen.so)" OFF)

option(USE_BACKTRACE "Enable backtrace() for debug builds (not always available)." ON)
option(USE_LIBEXECINFO
       "Use libexecinfo. For systems that don't have backtrace in libc." OFF)

# TODO: disable when not building for linux
option(PASSGEN_MLOCK "Use mlock() to lock memory on startup, preventing leaks through swapping" ON)

option(PASSGEN_SECCOMP "Build with seccomp filters" OFF)

option(PASSGEN_MONOCYPHER "Build with monocypher" ON)
option(PASSGEN_FUZZ "Build fuzzing harnesses with libFuzzer (needs Clang)" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(PASSGEN_DEBUG "Include debugging features for passgen" ON)
else()
    option(PASSGEN_DEBUG "Include debugging features for passgen" OFF)
endif()

# the tests, the benchmarks and the tools use the internals of the library,
# which the shared library does not export, so they are linked against the
# static library. so is the command-line interface by default, although it
# only uses the public interface (see PASSGEN_CLI_SHARED).
set(PASSGEN_LIB passgen-static)

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

# where libraries are installed. this is what GNUInstallDirs calls it, so that
# packaging for distributions which put them elsewhere (such as lib64) works
# as usual, but it is lib unless set. (a string rather than a path, as cmake
# would make a relative path given on the command line absolute.)
set(CMAKE_INSTALL_LIBDIR "lib"
    CACHE STRING "Where to install libraries, relative to the prefix")

# like GNUInstallDirs, it may also be absolute, as long as it is in the prefix
# (nix does that). what is installed only uses it relative to the prefix, so
# that the prefix can still be changed when installing.
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    file(RELATIVE_PATH PASSGEN_INSTALL_LIBDIR
        "${CMAKE_INSTALL_PREFIX}" "${CMAKE_INSTALL_LIBDIR}")
    if(PASSGEN_INSTALL_LIBDIR STREQUAL "" OR PASSGEN_INSTALL_LIBDIR MATCHES "^\\.\\.")
        message(FATAL_ERROR
            "CMAKE_INSTALL_LIBDIR (${CMAKE_INSTALL_LIBDIR}) has to be in "
            "CMAKE_INSTALL_PREFIX (${CMAKE_INSTALL_PREFIX})")
    endif()
else()
    set(PASSGEN_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
endif()

# the other directories, such as where the configuration of the system is
# (which is /etc for the prefix /usr).
include(GNUInstallDirs)

# the command-line tool and the pkg-config file find the library relative to
# where they are, so that they work wherever they are installed or unpacked.
file(RELATIVE_PATH PASSGEN_BIN_TO_LIBDIR
    "/prefix/bin" "/prefix/${PASSGEN_INSTALL_LIBDIR}")
file(RELATIVE_PATH PASSGEN_PKGCONFIG_TO_PREFIX
    "/prefix/${PASSGEN_INSTALL_LIBDIR}/pkgconfig" "/prefix")
string(REGEX REPLACE "/$" "" PASSGEN_PKGCONFIG_TO_PREFIX
    "${PASSGEN_PKGCONFIG_TO_PREFIX}")

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

    # the public headers are checked from C++ as well, if asked to. this is
    # not done by default, as the flags for the C compiler (such as for
    # sanitizers or coverage) are used for the C++ compiler too, and the C++
    # compiler which is found may not be the one which goes with it (g++ with
    # clang, or one for the build machine when cross compiling).
    if(PASSGEN_TEST_CXX)
        enable_language(CXX)
    endif()

    add_subdirectory(src/tests)

    add_test(NAME passgen-test COMMAND passgen-test)
    if(LIBPASSGEN_SHARED)
        add_test(NAME passgen-test-public COMMAND passgen-test-public)
    endif()
    if(TARGET passgen-test-public-cxx)
        add_test(NAME passgen-test-public-cxx COMMAND passgen-test-public-cxx)
    endif()

    # the examples, which have to keep working: with the default pattern,
    # with a wordlist, and with a pattern which is not valid.
    if(LIBPASSGEN_SHARED)
        add_subdirectory(examples)
        add_test(NAME passgen-example-generate
                 COMMAND passgen-example-generate)
        add_test(NAME passgen-example-generate-wordlist
                 COMMAND passgen-example-generate "\\w{words}-\\m{words}"
                         ${CMAKE_SOURCE_DIR}/examples/words.txt)
        add_test(NAME passgen-example-generate-invalid
                 COMMAND passgen-example-generate "[a-z")
        set_tests_properties(passgen-example-generate-invalid
                             PROPERTIES PASS_REGULAR_EXPRESSION
                             "error: set is not closed")
    endif()
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

# the version of the packages is that of the project, unless it is set to
# something else. nightly builds do that, to tell them apart from releases
# (see scripts/nightly-version.sh).
set(PASSGEN_PACKAGE_VERSION
    ""
    CACHE STRING "Version of the packages, defaults to the version of passgen.")
if(PASSGEN_PACKAGE_VERSION)
    set(CPACK_PACKAGE_VERSION ${PASSGEN_PACKAGE_VERSION})
else()
    set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION})
endif()

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


# RPM package, with everything in it, like the Debian package. What it needs
# (glibc, and the library for the command-line tool) is worked out by rpm.
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
# directories which the system has, rather than this package.
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr/share/man
    /usr/share/man/man1
    /usr/${PASSGEN_INSTALL_LIBDIR}
    /usr/${PASSGEN_INSTALL_LIBDIR}/pkgconfig)

include(CPack)


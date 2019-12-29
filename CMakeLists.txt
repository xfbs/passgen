cmake_minimum_required(VERSION 3.10)

project(passgen
    VERSION 1.0
    DESCRIPTION "Very nice project"
    LANGUAGES C)

option(BUILD_LIBPASSGEN "Build libpassgen. Enabled by default, if disabled uses system-wide libpassgen." ON)
option(BUILD_PASSGEN "Build passgen main binary." ON)
option(BUILD_TESTS "Build unit tests." ON)
option(BUILD_UTILS "Build utility executables." OFF)
option(BUILD_FUZZER "Build fuzzer using afl (requires setting compiler to afl-gcc)." OFF)

if(BUILD_LIBPASSGEN)
add_subdirectory(libpassgen)
endif()

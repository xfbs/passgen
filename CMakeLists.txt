cmake_minimum_required(VERSION 3.10)

project(passgen
    VERSION 1.0.0
    DESCRIPTION "Very nice project"
    LANGUAGES C)

# project build options.
option(BUILD_LIBPASSGEN "Build libpassgen. Enabled by default, if disabled uses system-wide libpassgen." ON)
option(BUILD_PASSGEN "Build passgen main binary." ON)
option(BUILD_TESTS "Build unit tests." ON)
option(BUILD_UTILS "Build utility executables." OFF)
option(BUILD_FUZZER "Build fuzzer using afl (requires setting compiler to afl-gcc)." OFF)
option(BUILD_GIT_INFO "Bake git commit information into the binaries." ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# get commit info, if git is available.
if(BUILD_GIT_INFO)
    include(GitInfo)
    git_info()
endif()

if(BUILD_LIBPASSGEN)
add_subdirectory(libpassgen)
endif()

if(BUILD_TESTS)
add_subdirectory(tests)
endif()

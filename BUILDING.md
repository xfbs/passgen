# Building

This project uses the CMake build system. Basic building is done by creating a build folder, running CMake to set up the build system, and then executing it.

    mkdir build
    cd build
    cmake ..
    make

When setting up the build system with CMake, it can be given 

## Dependencies

To build, some dependencies are necessary.

- C compiler (clang or gcc are recommended)
- cmake (at least 3.10)
- build system (make or ninja are recommended)
- libjansson
- libexecinfo (when building for musl instead of libc, such as with Alpine Linux)
- ruby
- git (optional)
- coverage reporting tool (llvm-cov or lcov, optional)
- libutf8proc (optional)

On Ubuntu or Debian, the base dependencies can be installed by running the following.

    sudo apt install gcc make cmake libjansson-dev ruby

On other systems, these packages should also be easily available.

## Setting Compiler

The compiler to use can be set with the `CC` environment variable. Recommended compilers are `gcc` and `clang`.

    CC=clang cmake ..

## Build types

CMake supports different build types. The *Debug* build type is recommended for development, this enables some extra checks in the code that are not normally present in the release build. These can be set with the `CMAKE_BUILD_TYPE` variable.

    cmake -DCMAKE_BUILD_TYPE=Debug

Other supported values are *Release* (enables optimisations), *RelWithDebInfo* (release mode with debug info) and *MinSizeRel*.

## Enabling Sanitizers

There is also support for enabling LLVM Sanitizers. This can be done by setting the `USE_SANITIZER` variable. These sanitizers add comprehensive checks at runtime at the expense of speed and memory usage.

    cmake -DUSE_SANITIZER=Address

Supported Sanitizers and Sanitizer combinations are *Address*, *Memory*, *MemoryWithOrigins*, *Undefined*, *Thread*, *Leak*, *Address;Undefined*.

## Formatting Code

If `clang-format` is present on the system, the *clangformat* target can be used to format the code.

    cmake ..
    make clangformat

This is recommended to do before commiting code changes.

## Formatting CMake Lists

The `CMakeLists.txt` that are part of the build system can be formatted as well. For this, cmake-format needs to be present on the system. This can be installed by running

    pip install cmake_format

Then, to run it, simply run

    make cmakeformat

Which will automatically format the files of the CMake build system. This only needs to be done if they are changed.

## Running Include-What-You-Use

## Running Tests

Tests can be run by using the `test` target of the build system.

    cmake ..
    make test

These should be run and pass before committing.

## Generating Test Coverage

Enabling test coverage requires either the GCC or Clang compilers and having the LLVM coverage tools installed (`lcov`, `llvm-cov`, `llvm-profdata`). Enabling it is done by setting `CODE_COVERAGE` to a true value.

This enables the `ccov-passgen-test` target, which runs tests and generates a test coverage report.

    cmake -DCODE_COVERAGE=YES ..
    make ccov-passgen-test

The resulting coverage is placed in `ccov/passgen-test/index.html` in the build folder.

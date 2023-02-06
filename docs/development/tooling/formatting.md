# Formatting

## Formatting Code

If `clang-format` is present on the system, the *clangformat* target can be
used to format the code.

    cd build
    make clangformat

This is recommended to do before committing code changes.

## Formatting CMake Lists

The `CMakeLists.txt` that are part of the build system can be formatted as
well. For this, cmake-format needs to be present on the system. This can be
installed by running

    pip install cmake_format

Then, to run it, simply run

    make cmakeformat

Which will automatically format the files of the CMake build system. This only
needs to be done if they are changed.


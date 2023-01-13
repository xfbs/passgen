# Tooling

The repository has some tooling that automates certain actions. For example, it can use `clang-format` to automatically format the source code, use Include-What-You-Use to clean up the header file includes, use LLVM sanitizers to check for code correctness and undefined behaviour, among other things. 

## Running Tests

Tests can be run by using the `test` target of the build system.

    make test

These should be run and pass before committing.

You can also filter the tests to run only specific tests by calling the test
binary directly. For example, this will only run tests that start with
`random`:

    ./src/test/passgen-test random

See the [Testing](testing.md) page for more information on the unit tests.

## Enabling Sanitizers

There is also support for enabling LLVM Sanitizers. This can be done by setting
the `USE_SANITIZER` variable. These sanitizers add comprehensive checks at
runtime at the expense of speed and memory usage.

    cmake -DUSE_SANITIZER=Address

Supported Sanitizers and Sanitizer combinations are *Address*, *Memory*,
*MemoryWithOrigins*, *Undefined*, *Thread*, *Leak*, *Address;Undefined*.

There is a script for compiling the code with a sanitizer in a temporary folder
and running tests, this can be done by running the following, using the Address
sanitizer for example:

    ./scripts/run_sanitizer.sh Address

It is useful to turn on these sanitizers when running unit tests, because they
catch more bugs that way. It is not recommended to leave the sanitizers enabled
for release builds of passgen, because they do add overhead (depending on the
sanitizer).

## Formatting Code

If `clang-format` is present on the system, the *clangformat* target can be
used to format the code.

    cmake ..
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

## Running Include-What-You-Use

Set the option `ENABLE_IWYU` to YES to turn it on. For example, when setting up
the build folder, run something like

    cmake -S . -B build-iwyu -DENABLE_IWYU=YES
    cmake --build build-iwyu

To generate a build folder and do the build with include-what-you-use
activated.

If you just want to run Include-What-You-Use and apply the suggested fixes,
there is a bash script that will do just that. Run

    ./scripts/fix-includes.sh

From within the root of the project, it will generate a build folder in a
temporary directory, run include-what-you-use on all files, and apply the fixes
automatically.

## Checking Symbols

The library should only export symbols starting with `passgen_` in order to
avoid conflicts with other libraries.

There is a target to check for this, simply running the following:

    make check-symbols

If there are symbols that don't match the expected name, an error message and
the name of the file will be printed.

## Generating Test Coverage

Enabling test coverage requires either the GCC or Clang compilers and having
the LLVM coverage tools installed (`lcov`, `llvm-cov`, `llvm-profdata`).
Enabling it is done by setting `CODE_COVERAGE` to a true value.

This enables the `ccov-passgen-test` target, which runs tests and generates a
test coverage report.

    cmake -DCODE_COVERAGE=YES ..
    make ccov-passgen-test

The resulting coverage is placed in `ccov/passgen-test/index.html` in the build
folder.

## Generating Documentation

Documentation is generated using doxygen. Both `doxygen` and the `graphviz`
packages need to be installed for it to work. To generate it, run doxygen in
the root of the project:

    doxygen Doxyfile

The resulting documentation is placed in `docs/html`.

## Packaging

CPack is supported for generating packages from the built code.

    cpack -G TXZ
    cpack -G DEB

These are used in the CI pipeline to create nightly releases.

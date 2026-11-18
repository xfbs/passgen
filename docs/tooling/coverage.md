# Coverage

In order for the unit tests to be effective, they must cover as much of the
codebase as possible. For this reason, Passgen includes some tooling that can
be used to compute how much of the codebase is executed by the unit tests. This
data is important for writing comprehensive tests.

## Quick

In order to quickly compute coverage, the root makefile in the repository has
a `coverage` target that creates a temporary build folder with coverage reporting
enabled and runs the unit tests.

    make coverage

The output of this command will tell you where the generated coverage report is
located.

## Using CMake

Enabling test coverage requires either the GCC or Clang compilers and having
the LLVM coverage tools installed (`lcov`, `llvm-cov`, `llvm-profdata`).
Enabling it is done by setting `CODE_COVERAGE` to a true value.

This enables the `ccov-passgen-test` target, which runs tests and generates a
test coverage report.

    cmake -DCODE_COVERAGE=YES ..
    make ccov-passgen-test

The resulting coverage is placed in `ccov/passgen-test/index.html` in the build
folder.

Monocypher (`src/monocypher.c` and `src/include/passgen/monocypher.h`) is left
out of the reports. It is vendored and tested upstream, and would otherwise make
up a large part of the report without saying anything about Passgen. The
patterns for this are in `passgen_target_defaults()` in `cmake/Passgen.cmake`.

On macOS, `llvm-cov` and `llvm-profdata` come with Xcode, but are not on the
`PATH`:

    PATH="$(dirname "$(xcrun --find llvm-cov)"):$PATH" CC=clang ./scripts/coverage.sh build-coverage

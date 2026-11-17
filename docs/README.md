# Developer documentation

This is the documentation for working on Passgen itself. For how to use it,
see the website at [passgen.io](https://passgen.io), and the man page
(`man/passgen.1.in`).

Passgen is written in C, and consists of a library (`libpassgen`) and a
command-line tool (`passgen`) which uses it. The library does not use any
global state, so it can be used from more than one thread, as long as every
thread uses its own objects. It is built with [CMake](https://cmake.org/), and
tested with GCC and Clang.

| Topic | |
| --- | --- |
| [Building](building.md) | Dependencies, how to build, and the build options. |
| [Testing](tooling/testing.md) | The unit tests and the tests of the command-line tool. |
| [Fuzzing](tooling/fuzzing.md) | The fuzzing harnesses, and how to run them. |
| [Sanitizers](tooling/sanitizers.md) | Building with the LLVM sanitizers. |
| [Coverage](tooling/coverage.md) | Generating test coverage reports. |
| [Benchmarks](tooling/benchmarks.md) | The benchmark tool. |
| [Formatting](tooling/formatting.md) | Formatting the code with `clang-format`. |
| [Includes](tooling/includes.md) | Cleaning up includes with Include-What-You-Use. |
| [Symbols](tooling/symbols.md) | Checking which symbols the library exports. |
| [Packaging](tooling/packaging.md) | Building packages with CPack. |
| [Documentation](tooling/documentation.md) | Where the documentation lives, and how to generate the API documentation. |
| [CI/CD](tooling/ci.md) | What runs in continuous integration, and how releases and nightly builds are made. |

## Where things are

The code is hosted in two places. Development happens on
[GitLab](https://gitlab.com/passgen/passgen), which runs the tests on every
commit, and publishes the nightly builds. It is mirrored to
[GitHub](https://github.com/xfbs/passgen), which is where releases are
published and where [bugs are reported](https://github.com/xfbs/passgen/issues).
[Merge requests](https://gitlab.com/passgen/passgen/-/merge_requests) are
welcome.

| | |
| --- | --- |
| [passgen.io](https://passgen.io) | The website, with the documentation for users. It has its own [repository](https://gitlab.com/passgen/website). |
| [API documentation](https://passgen.gitlab.io/passgen/docs/) | Generated from the headers with Doxygen, for the `master` branch. |
| [Coverage (lcov)](https://passgen.gitlab.io/passgen/coverage/lcov/), [coverage (llvm-cov)](https://passgen.gitlab.io/passgen/coverage/llvm-cov/) | Test coverage reports, for the `master` branch. |
| [Nightly builds](https://passgen.io/install/) | Built from the `master` branch, and signed. They are published at `passgen.gitlab.io/passgen/nightly/`, and listed on the website. |
| [Releases](https://github.com/xfbs/passgen/releases) | Numbered releases. |
| [Pipelines](https://gitlab.com/passgen/passgen/-/pipelines) | The state of continuous integration. |

## Layout of the repository

| Folder | Content |
| --- | --- |
| `cmake/` | CMake modules. |
| `data/` | The example configuration, the public key which nightly builds are signed with, and the wordlists (once downloaded with `scripts/fetch-wordlists.sh`). |
| `docs/` | This documentation. |
| `include/passgen/` | The public headers of `libpassgen`. |
| `include/passgen/container/` | Data structures. |
| `include/passgen/parser/` | The tokenizer and the parser for patterns. |
| `include/passgen/pattern/` | What patterns are parsed into. |
| `include/passgen/util/` | Utilities, such as UTF-8 and SipHash. |
| `man/` | The man page. |
| `scripts/` | Scripts, most of which are run by continuous integration. |
| `src/` | The sources of `libpassgen`, laid out like the headers. |
| `src/bench/` | The benchmark tool, and the benchmarks. |
| `src/cli/` | The sources of `passgen`, the command-line tool. |
| `src/fuzz/` | The fuzzing harnesses, and their corpus. |
| `src/tests/` | The unit testing framework (`tests.h` and `tests.c`), and the unit tests. |
| `src/tools/` | Small tools for trying out parts of the library. |

Code which was not written for Passgen is listed in
[THIRD_PARTY.md](../THIRD_PARTY.md), with its license.

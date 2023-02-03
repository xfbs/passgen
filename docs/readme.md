# Passgen Documentation

This is the documentation for the passgen project, a password generator.
Passgen is able to generate secure passwords and other random sequences of any
shape using a pattern that uses a syntax similar to that of [regular
expressions][regex].

Passgen currently consists of a command-line utility `passgen` that provides a
simple front-end to it and a library `libpassgen` that can be used to embed it
into other projects. This documentation is for the library `libpassgen`.

[regex]: https://en.wikipedia.org/wiki/Regular_expression

## Navigation

This is an overview of how the Passgen project is laid out on the filesystem:

| Folder | Content |
| --- | --- |
| `cmake/` | [CMake][] configuration. |
| `data/` | Static data for Passgen, such as default configuration. |
| `docs/` | Documentation for the Passgen project, hosted on [passgen.it][]. |
| `extern/` | External vendored dependencies. |
| `extern/utf8proc/` | Vendored version of [utf8proc][], a UTF-8 decoding library. |
| `include/` | Public header files. |
| `include/passgen/` | Header files of `libpassgen`. |
| `include/passgen/container/` | Data structure header files. |
| `include/passgen/parser/` | Parser header files. |
| `include/passgen/pattern/` | Pattern header files. |
| `include/passgen/util/` | Utility header files. |
| `man/` | Manual pages for Passgen. |
| `scripts/` | Utility scripts. Many of these are called from within the CI. These do not normally need to be called manually. |
| `src/` | All source files for `libpassgen`. |
| `src/bench/` | Benchmark utility (`bench.h` and `bench.c`) and all benchmarks for the Passgen project. |
| `src/cli/` | Source files for `passgen`, the command-line utility. |
| `src/container/` | Container-related source files of `libpassgen` |
| `src/parser/` | Parser-related source files of `libpassgen`. |
| `src/pattern/` | Pattern-related source files of `libpassgen`. |
| `src/tests/` | Unit testing framework (`tests.h` and `tests.c`) and all unit tests for `libpassgen`. |
| `src/tools/` | Various tools for passgen. |
| `src/util/` | Utility source files for `libpassgen`. |

[passgen.it]: https://passgen.it
[utf8proc]: http://juliastrings.github.io/utf8proc/
[CMake]: https://cmake.org/

For information on the grammar that passgen accepts, see [Syntax](../syntax).

For information on how to build the project, see [Building](../building).

For information on how the testing works, see [Testing](../testing).

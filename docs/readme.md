# Passgen Documentation

This is the documentation for the passgen project. This consists of both a library `libpassgen` and a binary, `passgen`. 

### Overview

The project is laid out into different parts.

- `tests/` contains the testing framework, comprised of `tests.h` and
  `tests.c`, as well as unit tests. Unit test files are listed in
  `tests/CMakeLists.txt`, and any files listed there are automatically scanned
  for test functions by `scripts/generate_test_list.rb` and added to the unit
  testing binary. That means that when adding tests functions, rebuilding will
  add it to the test list automatically, but adding more files require
  modifying the list of test files in the CMake config.
- `data/` contains word lists that are used to autogenerate markov chains (for
  word generation) by `scripts/generate_markov.rb`.
- `libpassgen/` contains the source code for the libpassgen logic.
- `libpassgen/data/` contains helper functions for data types (structs), such
  as initialisers and finalisers.
- `libpassgen/json/` contains functions that translate data structures into
  JSON and vice versa.
- `libpassgen/container/` contains files implementing container types, such as
  stacks of various data types.
- `include/passgen/` contains the header files for libpassgen.
- `include/passgen/data/` contains header files with definitions for data
  structures.
- `include/passgen/container/` contains header files for container
  implementations of various data structures.
- `bench/` contains the source code of the benchmark framework, `bench.h` and
  `bench.c`, as well as several benchmarks.
- `man/` contains manual pages for passgen.
- `tools/` contains useful utilities, such as to dump data structures out,
  show the sizes of various structs, testing assertions.
- `cmake/` contains helper files used for the CMake-based build system, for
  example to implement code formatting, code coverage generation and enabling
  other tools.

For information on the grammar that passgen accepts, see [Grammar](grammar.md).

For information on how to build the project, see [Building](building.md).

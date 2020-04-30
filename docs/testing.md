# Testing

The folder `tests/` contains the testing framework, comprised of `tests.h` and
`tests.c`, as well as unit tests in other C files. The unit testing system is a
very basic homegrown one, which only offers an `assert()` function.

Every file containing tests is listed in `tests/CMakeLists.txt`. Any test
sources listed there are automatically scanned by `scripts/generate_test_list.rb`,
which looks for function definitions such as

```c
test_result test_some_part(void) {
  // ...
}
```

At build time, a list of these functions is automatically generated in
`<build-folder>/tests/test_list.c`. This means that adding a new test function
only requires a rebuild to be recognised and added to the binary, whereas
adding a new test file requires editing the CMake config to make it get
scanned as well.

### Structure

The basic structure of a test should be that it needs to return `test_result`,
it's name should start with `test_`, it should not take any arguments and it
should return `test_ok`.

```c
test_result test_some_name(void) {
  // ...
  assert(2 == 2);
  // ...

  return test_ok;
}
```

### Tests binary

By default, when building the code, the tests binary is generated and found in
`<build-dir>/tests/passgen-test`. This should be run after every recompilation
to ensure that no breaking changes were introduced.

    $ ./build/tests/passgen-test
    ...
    [94/94] running utf8_encode_simple
    => 94/94 tests passed.

This tests binary accepts some arguments. Calling it with the `-h` flag shows
documentation on the options it accepts.

    $ ./build/tests/passgen-test -h
    ./tests/passgen-test [OPTIONS] [FILTERS...]
    Runs unit tests for passgen and reports results.

    Options
      -h, --help
        Shows this help text
      -v, --verbose
        Shows more verbose information
      -s, --shuffle
        Randomises the order in which tests are run

    Filters
      Match based on prefix. A filter such as 'random' would match any test case
      beginning with random, such as random_init, random_u8, etc.

In order to see more accurate timing information, you may want to run it with
the `--verbose` flag. The `--shuffle` flag is good to ensure that test cases
don't depend on each other.

The filters can be used to only run tests starting with a specific name, which
is not really necessary as running all tests only takes a fraction of a second
anyways, but can be useful when certain tests are broken during a refactoring.

### Recommendations

It usually makes sure to run tests with some kind of sanitizer, such as valgrind.
This catches illegal memory accesses.

    $ valgrind ./tests/passgen-test

Another recommendation is using the LLVM sanitizers. To do this, the code needs
to be compiled specially with them enabled, instructions on how to do that can
be found at [Building](building.md).

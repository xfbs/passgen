# Testing

When the `BUILD_TESTING` option is enabled on the CMake build system (which it
is, by default), a `passgen-test` binary will be built which runs unit tests
against the codebase.

For convenience, tests can be run using the `test` target of the build system.

    make test

These should be run and pass before committing.

You can also filter the tests to run only specific tests by calling the test
binary directly. For example, this will only run tests that start with
`random`:

    ./src/test/passgen-test random

### Recommendations

It is recommended to run the unit tests under [Valgrind][valgrind]
occasionally.  This will catch memory leaks and certain kinds of undesired
behaviour such as illegal memory accesses, at the expense of runtime speed.
When doing so, it is important to make sure that the tool reports no errors and
that "all heap blocks were freed".

    $ valgrind ./tests/passgen-test -v

Another recommendation is using the LLVM sanitizers. To do this, the code needs
to be compiled specially with them enabled, instructions on how to do that can
be found at the [Tooling](../tooling.md) page.

## Implementation

The folder [`src/tests/`][tests-folder] contains the testing framework, consisting of
[`tests.h`][tests.h] and [`tests.c`][tests.c] as well as the unit tests
themselves. The unit testing system is a very basic homegrown one which offers
`assert()` and `assert_eq()` macros and does some reporting.

Every file containing tests is listed in the [`CMakeLists.txt`][cmakelists] in the tests folder. Any test
sources listed there are automatically scanned for unit tests by 
[`scripts/generate_test_list.rb`][generate-test-list].

At build time, a list of these functions is automatically generated in
`<build-folder>/tests/test_list.c`. This means that adding a new test function
only requires a rebuild to be recognised and added to the binary, whereas
adding a new test file requires editing the CMake config to make it get
scanned as well.

### Structure

Every unit test must return the `test_result` type, and end with a `return
test_ok;` statement. The name of the unit test must start with `test_` and
should not take any arguments.

```c
test_result test_some_name(void) {
    assert_eq(2, 2);
    assert(true);

    return test_ok;
}
```

Any functions that do not match these requirements will not be picked up
automatically by the unit testing framework and will not run. This is so that
you can define helper functions that are not treated as tests themselves.

It is probably easiest to take a look at the other tests that already exist for
some reference on how to write your own unit tests.

### Tests binary

By default, when building the code, the tests binary is generated and found in
`<build-dir>/src/tests/passgen-test`. This should be run after every
recompilation to ensure that no breaking changes were introduced.

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

[tests-folder]: https://gitlab.com/xfbs/passgen/-/tree/master/src/tests
[tests.h]: https://gitlab.com/xfbs/passgen/-/blob/master/src/tests/tests.h
[tests.c]: https://gitlab.com/xfbs/passgen/-/blob/master/src/tests/tests.c
[cmakelists]: https://gitlab.com/xfbs/passgen/-/blob/master/src/tests/CMakeLists.txt
[generate-test-list]: https://gitlab.com/xfbs/passgen/-/blob/master/scripts/generate-test-list.rb
[valgrind]: https://valgrind.org/

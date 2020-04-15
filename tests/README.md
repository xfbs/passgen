# Tests

Tests live in the `tests/` folder in the source tree. The files `generate.rb`,
`tests.c` and `tests.h` are part of the testing framework and don't need to be
touched, unless the testing framework needs to be adapted. All other files
contain tests. 

## Executing tests

When building the project, running the binary `<build-folder>/tests/passgen-tests`
runs all the tests. If the environment variable SHUFFLE is set to 1, the list
of tests is shuffled before running them, which should be done to make sure
that test cases don't depend on each other.

## Adding a test

Every test looks like

```c
test_result test_name(void) {
    assert(4 == 4);

    return test_ok;
}
```

To add a new test, just add a new function with this layout to an existing
tests file. It will be picked up automatically by `scripts/generate_test_list.rb`.

Make sure that no two test functions have the same name, otherwise there will
be linker errors when compiling.

Tests should not depend on each other. Also, tests should not be nondeterministic,
meaning that if you use random data, use it with a constant seed. For practical
purposes, that means using `random_new_xorshift()` instead of `random_new()`.

## Adding a file

When adding a new file, make sure it includes `tests.h`. An example of a test
file, `mytests.c` could look like this:

```c
#include "tests.h"

test_result test_mytests_example(void) {
  assert((1 + 1) == 2);

  return test_ok;
}
```

In order for the file to be picked up, you need to add it to the `TEST_LIST`
in `tests/CMakeLists.txt`. This allows it to be compiled.

```cmake
set(TEST_LIST
    "testa.c"
    "testb.c"
    "mytests.c")
```

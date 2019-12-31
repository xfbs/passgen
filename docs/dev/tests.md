# Tests

This project has tests, located in the `tests/` directory. They are built by default and use a custom testing framework (to be replaced with CMocka at some point). When adding new testing functions, one needs to run

    cd tests
    ruby generate.rb all.c *.c

To regenerate `all.c`, which holds a list of all the testing functions to be executed.
This is not done automatically by cmake to reduce build dependencies (Ruby).

# Symbols

The library should only export symbols starting with `passgen_` in order to
avoid conflicts with other libraries.

There is a target to check for this, simply running the following:

    cd build
    make check-symbols

If there are symbols that don't match the expected name, an error message and
the name of the file will be printed.


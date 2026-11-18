# Packaging

CPack is supported for generating packages from the built code.

    cpack -G TXZ
    cpack -G DEB

These are used in the CI pipeline to create nightly releases.

## pkg-config

Along with the headers, a pkg-config file is installed as
`lib/pkgconfig/passgen.pc`, so that other build systems can find the library:

    pkg-config --cflags --libs passgen

It finds the prefix from where it is (`${pcfiledir}`), so it works wherever
the library is installed or an archive is unpacked. Linking statically needs
`--static`, which adds the math library.

`scripts/test-pkg-config.sh` installs a build into a temporary prefix, and
builds and runs `examples/generate.c` with only what pkg-config says. CI on
GitHub runs it.

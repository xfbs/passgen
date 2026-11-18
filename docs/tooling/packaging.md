# Packaging

CPack is supported for generating packages from the built code.

    cpack -G TXZ
    cpack -G DEB

These are used in the CI pipeline to create nightly releases.

## Releases

Releases are built by `.github/workflows/release.yml` when a tag like `v1.2.3`
is pushed. It can also be started by hand, which builds everything without
publishing it. A release has:

| File | What it is |
| --- | --- |
| `passgen-VERSION-linux-ARCH-static.tar.xz` | Only the command-line tool (and its man page), linked statically against musl. Runs on any Linux. |
| `passgen-VERSION-linux-ARCH.tar.xz` | The library (shared and static), its headers and pkg-config file, and the command-line tool linked against the shared library. Built on Ubuntu 22.04 against glibc. |
| `passgen_VERSION_ARCH.deb` | The same, as a Debian package. |
| `passgen-VERSION-macos-arm64.tar.xz` | The same, for macOS. |
| `passgen-VERSION-source.tar.gz` | The source. |

The archives are made by `scripts/package-archive.sh`, which installs the
components of a build which it is given (or all of them) into a directory, and
packs that up. The command-line tool is in the `runtime` component, along with
the shared library and the man page. The headers, the static library and the
pkg-config file are in `development`, and the wordlists are in `data`.

The command-line tool finds the shared library relative to where it is (see
`PASSGEN_CLI_SHARED` in [building](../building.md)), so the archives work
wherever they are unpacked. `scripts/test-relocatable.sh` checks that.

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

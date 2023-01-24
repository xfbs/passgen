# Building

<figure markdown>
![Building Passgen](media/building.svg){ width="600px" }
<figcaption>Building passgen from source</figcaption>
</figure>

If you want to work on Passgen, you can compile it from source. If you just want to 
try it, you can download pre-built and signed binaries from the [Releases](releases.md) page.

## Dependencies

To build, some dependencies are necessary.

- C compiler (for example `clang` or `gcc`),
- CMake (at least v3.10),
- Build system (for example `make` or `ninja`),
- libexecinfo (when building for musl instead of libc, such as with Alpine Linux),
- Ruby,
- Git (optional),
- Coverage reporting tool (`llvm-cov` or `lcov`, optional),
- [libutf8proc][utf8proc] (optional).

On Ubuntu or Debian, the base dependencies can be installed by running the following.

    sudo apt install gcc make cmake libjansson-dev ruby

On Alpine Linux, this should do the trick:

    apk add build-base cmake make jansson-dev git ruby llvm clang bash

On other systems, these packages should also be easily available.

## Cloning the Repository

Before you can set up the build system, you must clone the Passgen repository.
You can do this using Git. If you want to use the bundled version of [libutf8proc][utf8proc],
which is recommended, you have to make sure to also update the Git submodules.

    git clone https://gitlab.com/xfbs/passgen
    git submodule update --init

[utf8proc]: https://juliastrings.github.io/utf8proc/

## Build System

This project uses the CMake build system. Basic building is done by creating a build folder, running CMake to set up the build system, and then executing it. For example:

    mkdir build
    cd build
    cmake ..
    make -j

The CMake-based build system can take some build configuration as command-line arguments. These are set using the `-D` flag. For example, to set the build type, you can run CMake like this:

    cmake -DCMAKE_BUILD_TYPE=Release ..

Note that you usually have to delete and recreate the build folder if you want to change settings. These are all of the settings that it takes:

| Setting | Description |
| --- | --- |
| `BUILD_UTF8PROC` | Build utf8proc, the library used to decode UTF-8. If set to false, it will attempt to use the system version of libutf8proc. |
| `BUILD_TOOLS` | Build utility binaries. These are only really needed for testing. |
| `BUILD_BENCH` | Build benchmark utilities. |
| `BUILD_TESTING` | Build the `passgen-test` executable, which will run unit tests. It is recommended to always build this and run it to make sure there are no obvious issues with the build. |
| `LIBPASSGEN_STATIC` | Build the static libpassgen library. |
| `LIBPASSGEN_SHARED` | Build the shared libpassgen library. |
| `PASSGEN_STATIC` | Link the passgen utility against the static library rather than the dynamic one. |
| `PASSGEN_SECCOMP` | Enable [seccomp][] filters. These will run at startup to lock passgen down into a sandbox-like mode, used to enhance security, but can be fragile. |
| `USE_SANITIZER` | Enables use of one of the [LLVM sanitizers][asan]. |
| `CODE_COVERAGE` | Enable instrumentation for generating code coverage reports. |

[asan]: https://clang.llvm.org/docs/AddressSanitizer.html
[seccomp]: https://www.kernel.org/doc/html/v4.19/userspace-api/seccomp_filter.html

## Overriding Compiler

CMake will pick out the default C compiler on your system automatically. If you have multiple compilers installed (such as `gcc` and `clang`, for example), this might not do what you want. You can override the C compiler used by setting the `CC` environment variable when you initially set up the build system.

    CC=clang cmake ..

## Build types

CMake supports different build types. The *Debug* build type is recommended for development, this enables some extra checks in the code that are not normally present in the release build. These can be set with the `CMAKE_BUILD_TYPE` variable.

    cmake -DCMAKE_BUILD_TYPE=Debug

The supported build types are:

| Type | Description |
| --- | --- |
| Release | Builds with optimisations. |
| Debug | Builds without optimizations. |
| RelWithDebInfo | Build with optimisations. |
| MinSizeRel | Minimal executable size. |

## After Building

You may want to run tests after building:

    make test
	# or
	./src/test/passgen-test -v

If the tests work, you can use the build passgen:

    ./src/cli/passgen -h

If you want to install the version of Passgen that you have installed locally, it is recommended that you do so by building a package from it and installing that package. That way, you get all of the data (man page, libraries, CLI) and it is simple to remove it again.

    cpack -G DEB
    apt install ./passgen_*_amd64.deb

See the [Tooling](tooling.md) section for more tooling options.

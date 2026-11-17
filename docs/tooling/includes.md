# Includes

In order for the build to stay lean, it is important that two properties are ensured:

- All header files include what they use. They should explicitly import headers
  they depend on, rather than relying on types being imported by other headers.
- No header file has unused includes. This can happen when refactoring, when
  one forgets to remove an include that is no longer used.

The [Include-What-You-Use][] tool is helpful for ensuring these properties. It
works by scanning the code base and automatically fixing headers.

[Include-What-You-Use]: https://include-what-you-use.org/

## Quick Run

If you just want to run Include-What-You-Use and apply the suggested fixes,
there is a bash script that will do just that. Run

    ./scripts/fix-includes.sh

From within the root of the project, it will generate a build folder in a
temporary directory, run include-what-you-use on all files, and apply the fixes
automatically.

## Using CMake

Set the option `ENABLE_IWYU` to YES to turn it on. For example, when setting up
the build folder, run something like

    cmake -S . -B build-iwyu -DENABLE_IWYU=YES
    cmake --build build-iwyu

To generate a build folder and do the build with include-what-you-use
activated.

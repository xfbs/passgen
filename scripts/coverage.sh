#!/bin/bash
set -euo pipefail

if test "$#" -lt 1; then
    echo "$0 <build-dir> [<path to source>]"
    echo
    echo "Builds the code with code coverage support. This only works when using either"
    echo "gcc or clang. The coverage from running the specified binary is left in the"
    echo "build dir in ccov/."
    echo
    echo "To use a different compiler, set the CC env variable:"
    echo
    echo "    CC=clang ./scripts/coverage.sh build-clang"
    echo "    CC=gcc ./scripts/coverage.sh build-gcc"
    echo
    echo "At the very end of the build process, there should be a line telling you where"
    echo "the generated HTML coverage report is on the filesystem."
    exit 1
fi

source_dir="${2:-$(pwd)}"
outdir=$1

# initialize build dir
# I don't know why, but generated the coverage doesn't work when using Ninja as
# build system, only with Makefiles. Investigate?
# build passgen statically (link to libpassgen.a rather than libpassgen.so),
# that way coverage reporting is more accurate.
cmake -S "$source_dir" -B "$outdir" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCODE_COVERAGE=YES \
    -DBUILD_GIT_INFO=NO \
    -DPASSGEN_STATIC=Yes

# compute coverage from running unit tests
make -C "$outdir" -j ccov-passgen-test

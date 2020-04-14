#!/bin/bash
set -euo pipefail

# I don't know why, but generated the coverage doesn't work when using Ninja as
# build system, only with Makefiles. Investigate?
function gen_coverage() {
  source_dir="${3:-$(pwd)}"
  binary="$1"
  outdir=$2

  mkdir "$outdir"
  cd "$outdir"

  ctest --build-and-test \
    "$source_dir" \
    "$outdir" \
    --build-generator "Unix Makefiles" \
    --build-options \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCODE_COVERAGE=YES \
    -DBUILD_GIT_INFO=NO \
    --test-command \
    make "ccov-$binary"
}

if test "$#" -lt 1; then
  echo "$0 <binary> <build-dir> [<path to source>]"
  echo
  echo "Builds the code with code coverage support. This only works when using either"
  echo "gcc or clang. The coverage from running the specified binary is left in the"
  echo "build dir in ccov/."
else
  gen_coverage "$@"
fi

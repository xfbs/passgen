#!/bin/bash
set -euo pipefail

# I don't know why, but generated the coverage doesn't work when using Ninja as
# build system, only with Makefiles. Investigate?
function coverage() {
  source_dir="${2:-$(pwd)}"
  outdir=$1

  cmake -S "$source_dir" -B "$outdir" -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=YES -DBUILD_GIT_INFO=NO
  make -C "$outdir" -j ccov-passgen-test
}

if test "$#" -lt 1; then
  echo "$0 <build-dir> [<path to source>]"
  echo
  echo "Builds the code with code coverage support. This only works when using either"
  echo "gcc or clang. The coverage from running the specified binary is left in the"
  echo "build dir in ccov/."
else
  coverage "$@"
fi

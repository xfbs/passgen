#!/bin/bash
set -euo pipefail

function gen_coverage() {
  source_dir="${3:-$(pwd)}"
  binary="$1"
  tmp_dir=$2

  mkdir "$tmp_dir"
  cd "$tmp_dir"

  ctest --build-and-test \
    "$source_dir" \
    "$tmp_dir" \
    --build-generator Ninja \
    --build-options \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCODE_COVERAGE=YES \
    -DBUILD_GIT_INFO=NO \
    --test-command \
    ninja "ccov-$binary"
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

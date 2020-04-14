#!/bin/bash
set -euo pipefail

function run_sanitizer() {
  source_dir="${2:-.}"
  sanitizer="$1"
  tmp_dir=$(mktemp -d)

  ctest --build-and-test \
    "$source_dir" \
    "$tmp_dir" \
    --build-generator Ninja \
    --build-options \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUSE_SANITIZER=$sanitizer \
    -DBUILD_GIT_INFO=NO \
    --test-command ctest --output-on-failure

  rm -rf "$tmp_dir"
}

if test "$#" -lt 1; then
  echo "$0 <sanitizer> [<path to source>]"
  echo
  echo "Builds the code with the given sanitizer enabled in a temporary directory"
  echo "and runs the tests. Possible values for the sanitizer are:"
  echo
  echo "  Address Memory Undefined Leak Thread"
  echo
  echo "If the path to the source directory is not given, the script assumes it is"
  echo "in the current working directory."
  echo
  echo "For more information on the sanitizers, read cmake/Sanitizers.cmake"
else
  run_sanitizer "$@"
fi

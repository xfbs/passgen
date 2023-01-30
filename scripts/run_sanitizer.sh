#!/bin/bash
set -euxo pipefail

function run_sanitizer() {
    sanitizer="$1"
    sanitizer_lower="${sanitizer,,}"
    sanitizer_first="${sanitizer_lower:0:1}"
    build_dir="${2:-build-${sanitizer_first}san}"

    cmake -S . -B "$build_dir" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DUSE_SANITIZER=$sanitizer \
        -DBUILD_GIT_INFO=NO

    ninja -C "$build_dir" test
}

if test "$#" -lt 1; then
  echo "$0 <sanitizer> [<build-folder>]"
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

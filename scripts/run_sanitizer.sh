#!/bin/bash
# Run a sanitizer against the tests. Execute as
# ./scripts/run_sanitizer.sh <Sanitizer> [<Path to Source>]
# Valid sanitizers are: Address Memory Undefined Leak Thread

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

run_sanitizer "$@"

#!/bin/bash
set -euo pipefail

source_dir="$1"

function run_sanitizer() {
  sanitizer="$1"
  tmp_dir=$(mktemp -d)

  CC=clang ctest --build-and-test \
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

run_sanitizer "Address"
run_sanitizer "Undefined"
run_sanitizer "Memory"
run_sanitizer "Leak"

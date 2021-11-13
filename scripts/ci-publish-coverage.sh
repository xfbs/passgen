#!/bin/bash
set -euxo pipefail

mkdir -p public/coverage
mv build-clang/ccov/passgen-test public/coverage/llvm-cov
mv build-gcc/ccov/passgen-test public/coverage/lcov

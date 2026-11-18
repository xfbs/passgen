#!/bin/sh
# Installs the library from a build directory into a temporary prefix, and
# builds and runs the example against it, using only what pkg-config says.
# This checks that the installed passgen.pc, headers and library work for
# something outside of this repository.
#
# usage: test-pkg-config.sh <build directory> <C compiler>
set -eu

build=$1
cc=$2
source=$(cd "$(dirname "$0")/.." && pwd)
prefix=$(mktemp -d)
trap 'rm -rf "$prefix"' EXIT

cmake --install "$build" --prefix "$prefix" --component development > /dev/null
cmake --install "$build" --prefix "$prefix" --component runtime > /dev/null

export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
pkg-config --exists --print-errors passgen
echo "passgen $(pkg-config --modversion passgen)"

# the library is not on the search path of the loader, so the example is
# told where it is.
# shellcheck disable=SC2046
"$cc" -o "$prefix/generate" "$source/examples/generate.c" \
    $(pkg-config --cflags --libs passgen) -Wl,-rpath,"$prefix/lib"
"$prefix/generate" "[a-z]{8}"

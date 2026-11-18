#!/bin/sh
# Packages what a build installs as an archive (.tar.xz) in the current
# directory, which has a directory with the same name as the archive in it.
# Without components, all of them are packaged.
#
# usage: package-archive.sh <build directory> <name> [component...]
#
#     package-archive.sh build passgen-1.0.0-linux-x86_64-static runtime
set -eu

build=$1
name=$2
shift 2
out="$(pwd)/$name.tar.xz"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

if test "$#" -eq 0; then
    cmake --install "$build" --prefix "$work/$name" --strip > /dev/null
else
    for component in "$@"; do
        cmake --install "$build" --prefix "$work/$name" --component "$component" --strip > /dev/null
    done
fi

(cd "$work" && cmake -E tar cJf "$out" "$name")
echo "$out:"
(cd "$work" && find "$name" -type f -o -type l | sort | sed 's/^/  /')

#!/bin/sh
# Installs the command-line tool and the library from a build directory into
# a temporary prefix, moves that somewhere else, and runs the tool from there.
# A tool which is linked against the shared library has to find it relative
# to where it is, so that an archive of a release works wherever it is
# unpacked.
#
# usage: test-relocatable.sh <build directory>
set -eu

build=$1
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

cmake --install "$build" --prefix "$work/installed" --component runtime > /dev/null
mv "$work/installed" "$work/moved"

passgen="$work/moved/bin/passgen"
"$passgen" --version
test "$("$passgen" -r zero '[a-z]{8}')" = "aaaaaaaa"

# when linked against the shared library, it has to find the one which was
# installed along with it: the run path has to be relative to the binary, and
# not point into the build directory.
case "$(uname)" in
    Darwin)
        libraries="$(otool -L "$passgen" | tail -n +2)"
        runpath="$(otool -l "$passgen" | grep -A2 LC_RPATH | sed -n 's/^ *path \(.*\) (offset.*/\1/p')"
        origin='@loader_path'
        ;;
    *)
        libraries="$(ldd "$passgen" 2>&1 || true)"
        runpath="$(objdump -p "$passgen" | sed -n 's/^ *\(RUNPATH\|RPATH\) *//p')"
        origin='$ORIGIN'
        ;;
esac

if echo "$libraries" | grep -q libpassgen; then
    echo "linked against the shared library, with run path: $runpath"
    # relative to the binary, into the directory with the libraries (which
    # is not always lib), and nothing else.
    case "$runpath" in
        "$origin"/../*) ;;
        *) false ;;
    esac
    test -e "$work/moved/bin/$(echo "$runpath" | sed "s|^$origin/||")"
else
    echo "linked against the static library"
fi

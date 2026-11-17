#!/bin/bash
# Builds the fuzzing harnesses with libFuzzer and sanitizers, and runs them.
set -euo pipefail

if test "${1:-}" = "-h" || test "${1:-}" = "--help"; then
    echo "$0 [<seconds> [<harness>...]]"
    echo
    echo "Builds the fuzzing harnesses in build-fuzz/, and runs each of them for the"
    echo "given number of seconds (60 by default). Runs all harnesses, unless some are"
    echo "named: pattern, wordlist, random or utf8."
    echo
    echo "This needs a Clang which comes with libFuzzer. Set CC to choose which one to"
    echo "use, and BUILD_TYPE to fuzz a release build (Debug by default)."
    echo
    echo "Inputs which cause a problem are saved in build-fuzz/crashes/. The inputs"
    echo "which were found along the way are kept in build-fuzz/work/, so that running"
    echo "this again continues where it left off."
    exit 0
fi

seconds="${1:-60}"
if test "$#" -gt 0; then
    shift
fi
if test "$#" -gt 0; then
    harnesses="$*"
else
    harnesses="pattern wordlist random utf8"
fi

source_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$source_dir/build-fuzz"

# the clang which comes with Xcode does not have libFuzzer, the one from
# Homebrew does.
if test -z "${CC:-}"; then
    CC=clang
    for candidate in /opt/homebrew/opt/llvm/bin/clang /usr/local/opt/llvm/bin/clang; do
        if test "$(uname)" = "Darwin" && test -x "$candidate"; then
            CC="$candidate"
            break
        fi
    done
fi
export CC

# stop at the first finding of a sanitizer, rather than carrying on.
export CFLAGS="${CFLAGS:-} -fno-sanitize-recover=all"

cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-Debug}" \
    -DPASSGEN_FUZZ=ON \
    -DUSE_SANITIZER="Address;Undefined" \
    -DBUILD_BENCH=OFF \
    -DBUILD_TOOLS=OFF \
    -DBUILD_GIT_INFO=OFF

targets=""
for harness in $harnesses; do
    targets="$targets fuzz-$harness"
done
# shellcheck disable=SC2086
cmake --build "$build_dir" --target $targets

failed=""
for harness in $harnesses; do
    work="$build_dir/work/$harness"
    crashes="$build_dir/crashes/$harness"
    mkdir -p "$work" "$crashes"

    echo
    echo "=== fuzzing $harness for $seconds seconds"
    if ! "$build_dir/src/fuzz/fuzz-$harness" \
        "$work" "$source_dir/src/fuzz/corpus/$harness" \
        -dict="$source_dir/src/fuzz/pattern.dict" \
        -max_len=512 \
        -timeout=10 \
        -max_total_time="$seconds" \
        -artifact_prefix="$crashes/" \
        -print_final_stats=1; then
        failed="$failed $harness"
    fi
done

echo
if test -n "$failed"; then
    echo "Found problems in:$failed"
    echo "The inputs which cause them are in $build_dir/crashes/"
    exit 1
fi

echo "No problems found."

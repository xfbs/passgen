#!/bin/bash
if test "$#" -lt 1; then
    echo "$0 <passgen cli path>"
    echo "Runs tests against the passgen binary."
    echo
    echo "Example:"
    echo "    mkdir build"
    echo "    cmake -S . -B build"
    echo "    cmake --build build"
    echo "    ./scripts/test-passgen-cli.sh ./build/src/cli/passgen"
    exit 0
fi
set -euxo pipefail

# which binary to test
passgen=$@

function matches() {
    grep -E "^$1$"
}

function pattern_matches() {
    $passgen "$1" | matches "$1"
}

# test simple grammar
pattern_matches "a"
pattern_matches "a?"
pattern_matches "a{2}"
pattern_matches "a{1,2}"
pattern_matches "abc"
pattern_matches "abc|def"
pattern_matches "(abc)"
pattern_matches "(abc){2}"
pattern_matches "(abc){1,2}"
pattern_matches "(abc|def)"
pattern_matches "(abc|def){2}"
pattern_matches "(abc|def){1,2}"
pattern_matches "[a]"
pattern_matches "[ab]"
pattern_matches "[a-z]"
pattern_matches "[a-zA-Z0-9#]"
pattern_matches "[a-zA-Z0-9#]{2}"
pattern_matches "[a-zA-Z0-9#]{1,2}"

# test presets
$passgen -p apple1
$passgen -p apple2
$passgen -p firefox
$passgen -p uuid
$passgen -p edge

# test options
$passgen --amount 5 "a"
$passgen -a 5 "a"

$passgen --null "a"
$passgen -z "a"

$passgen --entropy "a"
$passgen -e "a"

$passgen --random "xorshift:1234" "a"
$passgen --random "zero" "a"
$passgen --random "system" "a"

# test flags
$passgen --version
$passgen -v

$passgen --help
$passgen -h

$passgen --list
$passgen -l

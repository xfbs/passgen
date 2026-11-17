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

# don't let the configuration of whoever runs this influence the tests
tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
export HOME="$tmpdir/home"
export XDG_CONFIG_HOME="$tmpdir/config"
unset XDG_CONFIG_PATH
mkdir -p "$HOME" "$XDG_CONFIG_HOME"

function matches() {
    grep -E "^$1$"
}

# things that should fail, and not print anything to standard output
function fails() {
    local output
    if output="$("$@" 2>/dev/null)"; then
        echo "expected failure: $*"
        exit 1
    fi
    test -z "$output"
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

# output is not limited in length
test "$($passgen "a{1000}" | tr -d '\n' | wc -c)" -eq 1000
test "$($passgen -a 3 "[a-z]{500}" | wc -l)" -eq 3

# large outputs work when only little memory may be locked. the subshell keeps
# the limit from applying to the rest of the tests, and this is skipped where
# the limit cannot be set to this value.
if (ulimit -l 8192) 2> /dev/null; then
    test "$(ulimit -l 8192; $passgen "a{20000000}" | tr -d '\n' | wc -c)" -eq 20000000
fi

# deterministic randomness sources always produce the same output
test "$($passgen -r xorshift:1234 "[a-z]{32}")" = "$($passgen -r xorshift:1234 "[a-z]{32}")"
test "$($passgen -r zero "[a-z]{8}")" = "aaaaaaaa"

# master passphrase mode is deterministic, and depends on all of its inputs.
# these only work when built with monocypher (the default).
if $passgen -r chacha20:key "a" > /dev/null 2>&1; then
    a="$($passgen -m master -d example.com -t user -p apple2)"
    test "$a" = "$($passgen -m master -d example.com -t user -p apple2)"
    test "$a" != "$($passgen -m other -d example.com -t user -p apple2)"
    test "$a" != "$($passgen -m master -d example.org -t user -p apple2)"
    test "$a" != "$($passgen -m master -d example.com -t admin -p apple2)"

    # known answer, to notice when the derivation changes (which would change
    # everyone's passwords).
    test "$a" = "JuIHep-ZdR5yQ-Nq7cO2"

    # deriving the key needs more memory than may commonly be locked
    if (ulimit -l 8192) 2> /dev/null; then
        test "$a" = "$(ulimit -l 8192; $passgen -m master -d example.com -t user -p apple2)"
    fi

    # overlong keys are rejected
    long="$(printf 'A%.0s' $(seq 1 300))"
    fails $passgen -r "chacha20:$long" "a"
    fails $passgen -r "chacha20-argon2:$long" "a"
fi

# JSON output
test "$($passgen -r zero --json "[a-z]{4}")" = '[{"output":"aaaa"}]'
test "$($passgen -r zero --json -a 2 'a"\\')" = '[{"output":"a\"\\"},{"output":"a\"\\"}]'
$passgen --json --entropy "[a-z]{4}" | matches '\[\{"output":"[a-z]{4}","entropy":18\.80[0-9]*\}\]'

# entropy
test "$($passgen -e "[a-z]{4}" 2>&1 >/dev/null)" = "entropy: 18.80 bits"

# wordlists
printf 'alpha\nbravo\ncharlie\n' > "$tmpdir/words.txt"
: > "$tmpdir/empty.txt"
$passgen -w "test:$tmpdir/words.txt" '\w{test}' | matches "(alpha|bravo|charlie)"
$passgen -w "test:$tmpdir/words.txt" '\w{test}(-\w{test}){3}' | matches "[a-z]+(-[a-z]+){3}"
$passgen -w "test:$tmpdir/words.txt" '\m{test}' | matches "[a-z]+"
$passgen -w "test:$tmpdir/words.txt" -M 2 '\m{test}' | matches "[a-z]+"

# user-defined presets and configuration file
test "$($passgen -P "mine:abc" -p mine)" = "abc"
echo "preset fromconfig:xyz" > "$XDG_CONFIG_HOME/passgen.conf"
test "$($passgen -p fromconfig)" = "xyz"
rm "$XDG_CONFIG_HOME/passgen.conf"
mkdir -p "$HOME/.config"
echo "preset fromhome:uvw" > "$HOME/.config/passgen.conf"
test "$(XDG_CONFIG_HOME= $passgen -p fromhome)" = "uvw"

# depth limit
$passgen -D 5 "((((a))))" | matches "a"
fails $passgen -D 2 "((((a))))"

fails $passgen "a{5,2}"
fails $passgen "(abc"
fails $passgen "[z-a]"
fails $passgen -a 0 "a"
fails $passgen -M 0 "a"
fails $passgen -M 1000 "a"
fails $passgen -D 0 "a"
fails $passgen -p doesnotexist
fails $passgen -P "nocolon" "a"
fails $passgen -r nonsense "a"
fails $passgen -r xorshift: "a"
fails $passgen -r "file:$tmpdir/doesnotexist" "a"
fails $passgen -w "nocolon" "a"
fails $passgen -w "test:$tmpdir/doesnotexist" "a"
fails $passgen -w "test:$tmpdir/empty.txt" '\w{test}'
fails $passgen -w "test:$tmpdir/empty.txt" '\m{test}'
fails $passgen '\w{undefined}'
fails $passgen --json '\w{undefined}'
fails $passgen --nonexistent-option
fails $passgen "a" "b"
fails $passgen -p apple1 "a"

set +x
echo "All CLI tests passed."

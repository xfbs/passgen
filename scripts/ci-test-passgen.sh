#!/bin/bash
set -euxo pipefail

# which binary to test
passgen=$@

# test simple grammar
$passgen "a"
$passgen "a{2}"
$passgen "a{1,2}"
$passgen "abc"
$passgen "abc|def"
$passgen "(abc)"
$passgen "(abc){2}"
$passgen "(abc){1,2}"
$passgen "(abc|def)"
$passgen "(abc|def){2}"
$passgen "(abc|def){1,2}"
$passgen "[a]"
$passgen "[ab]"
$passgen "[a-z]"
$passgen "[a-zA-Z0-9#]"
$passgen "[a-zA-Z0-9#]{2}"
$passgen "[a-zA-Z0-9#]{1,2}"

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

$passgen --random "xor:1234" "a"
$passgen --random "zero" "a"
$passgen --random "system" "a"

# test flags
$passgen --version
$passgen -v

$passgen --help
$passgen -h

$passgen --list
$passgen -l

#!/bin/sh
# Fetch Wordlists
#
# This is run in a plain Alpine container in CI, so it has to work with any
# POSIX shell, not just with bash.
set -eux

# fetch EFF wordlist
eff_fetch() {
    filename="$1"
    url="$2"
    trim="$3"

    # download first and trim after (rather than piping one into the other), so
    # that a failed download is noticed without needing `set -o pipefail`.
    curl -sf "$url" -o "data/$filename.download"
    cut -c"$trim"- "data/$filename.download" > "data/$filename.txt"
    rm "data/$filename.download"
}

eff_fetch "eff-large" "https://www.eff.org/files/2016/07/18/eff_large_wordlist.txt" 7
eff_fetch "eff-short-1" "https://www.eff.org/files/2016/09/08/eff_short_wordlist_1.txt" 6
eff_fetch "eff-short-2" "https://www.eff.org/files/2016/09/08/eff_short_wordlist_2_0.txt" 6

cd data && sha1sum -c checksums.txt

# Fetch Wordlists
set -euxo pipefail

# fetch EFF wordlist
function eff_fetch() {
    filename="$1"
    url="$2"
    trim="$3"

    curl -s "$url" | cut -c$trim- > "data/$filename.txt"
}

eff_fetch "eff-large" "https://www.eff.org/files/2016/07/18/eff_large_wordlist.txt" 7
eff_fetch "eff-short-1" "https://www.eff.org/files/2016/09/08/eff_short_wordlist_1.txt" 6
eff_fetch "eff-short-2" "https://www.eff.org/files/2016/09/08/eff_short_wordlist_2_0.txt" 6

cd data && sha1sum -c checksums.txt

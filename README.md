# Passgen [![CI Pipelines](https://gitlab.com/xfbs/passgen/badges/master/pipeline.svg)](https://gitlab.com/xfbs/passgen/-/pipelines) [![Documentation](https://xfbs.gitlab.io/passgen/badges/docs.svg)](https://passgen.it/docs/index.html)

Passgen is a tool to generate random passphrases from a regular expression. It
supports choosing words from a wordlist, building a markov-chain from a
wordlist to generate high-entropy, but pronounceable sequences. It can also be
used with a master passphrase.

* Website: [passgen.it](https://passgen.it)
* Source Documentation: [Doxygen](https://passgen.it/docs/index.html)
* Code Coverage: [lcov](https://passgen.it/coverage/lcov), [llvm-cov](https://passgen.it/coverage/llvm-cov)
* Continuous Integration: [Pipelines](https://gitlab.com/xfbs/passgen/pipelines)
* Releases: [Releases](https://passgen.it/releases)

## Features

Passgen can generate random sequences from a format string with a
regular-expression-like syntax.  This can be used to generate random data for
testing, or secure passphrases. Due to the flexible nature of regular
expressions, it lets you generate passphrases matching any possible security
policy. Here are some examples:

```
$ passgen '[a-z]{20}'
jficsnasvtkpjwcimitz
$ passgen '([a-zA-Z]{2}[0-9]){7}'
eJ4YP8bb4UH5Gk3Ue3ds3
```

If you are not comfortable with regular expressions, it also comes with a list
of preset formats that you can use:

```
$ passgen -p apple2
MKCvtR-N2YNJP-29KRae
$ passgen -p firefox
kmRxBcUCnvQNAL4
$ passgen -p proquint64
zabil-pukug-punud-lasup
```

For any sequence that Passgen generates, it can measure the entropy it took to
generate it, so that you have a reasonable estimation of how difficult it would
be to crack the sequence using a brute-force approach.

```
$ passgen -e '[a-c]{3}'
entropy: 4.75 bits
cab
passgen -e '[a-zA-Z0-9]{32}'
entropy: 190.53 bits
QjKV1q0DKZ8Pc2GM5ccQbYxXfwAbjECk
```

Passgen supports reading in wordlists. You can use this to generate
diceware-like passphrases, for example. Since it has full Unicode support, it
can read wordlist for any language.  Here are some examples:

```
$ passgen -w english:/usr/share/dict/words '\w{english}(-\w{english}){4}'
limelight-icicles-commoner-stamped-garnet
$ passgen -w german:/usr/share/dict/ngerman '\w{german}(-\w{german}){4}'
Handlinie-streitbareren-Fischleim-punktiere-Nationalteam
```

In addition to just selecting random words from wordlists, Passgen can also
build a [Markov-Chain](https://en.wikipedia.org/wiki/Markov_chain) of the
letter sequences and use that to generate words with more entropy, that are
still pronounceable.

```
$ passgen -w english:/usr/share/dict/words '\m{english}(-\m{english}){4}' -e
entropy: 117.37 bits
Eucleased-pendled-Villish-Corld's-capirators
$ passgen -w english:/usr/share/dict/words '\m{english}(-\m{english}){4}' -e
entropy: 118.00 bits
maltry-oblinkgo-Langkor-impularian's-vi
```

Every time you run Passgen, it generates completely random output using your
machine's source of secure randomness. But in some situations, you may want a
more deterministic output, for example because you want to generate passphrases
from a master passphrase.  For that reason, Passgen has some command-line
options that you can use to change the source of randomness it uses. In this
mode, Passgen will always generate the same passphrase when given the same
master passphrase as input. This means that you only need to remember a single
master passphrase, and you can generate any kind of passphrase from it.  Under
the hood, it uses [Argon2](https://en.wikipedia.org/wiki/Argon2) to derive a
key from your inputs, and the
[ChaCha20](https://en.wikipedia.org/wiki/Salsa20#ChaCha_variant) stream cipher
as a pseudorandom number generate using the derived key.

```
$ passgen -m mymasterpass -d example.com -t username -p apple2
joCRgX-ISiyaU-dc7VrC
```

## Syntax

The syntax of the Passgen format string is similar to that of regular expressions,
but it has some additional features. These are the operations that Passgen understands:

| Syntax | Explanation |
| --- | --- |
| `[a-z]`, `[a-zA-Z0-9]`, `[abcdefg]` | Range. Choose a letter at random. You can give it ranges (`a-z`) or individual letters. If you want to use the literal dash as a letter, either place it last or escape it with a backslash. |
| `(abc|def)` | Group. Choose one at random: `abc` or `def`. |
| `a{7}`, `[a-z]{4,6}` | Repetition. Appending a `{n}` to any syntax repeats the last thing *n* times. Appending a `{n,m}` to any syntax repeats the last thing a random amount between *n* and *m* times. If you want to repeat a whole sequence of things, put them into a group (in parentheses). |
| `\w{english}` | Word. Picks a random word from the wordlist with the name `english`. You can use the `-w name:/path/to/list` command-line option to load wordlists. You can load multiple wordlists. |
| `\m{english}` | Markov-Chain. Generates a high-entropy, but pronounceable word by building a Markov-Chain of the wordlist `english`. This allows you to make a tradeoff between memorability and strength: words are easier to remember than completely random sequences. The Markov chain lets you generate random sequences of letters that follow the same probability distribution as the words in the wordlist, meaning that they are pronounceable and thus easier to remember. |

## Building

In general, if you have all dependencies installed, doing the following should result in a working binary.

    $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    $ cmake --build build
    $ cmake --install build

See [Building](https://passgen.it/development/building) for more information about building the project.

You may also download [signed nightly releases](https://passgen.it/nightly/) from the website.

## Prior art

* [pwgen](https://linux.die.net/man/1/pwgen).
* [apg](https://linux.die.net/man/1/apg): generates random passwords, can either be generated such that they are pronounceable, or taken from a predefined list of symbols.
* [z-tokens](https://github.com/volution/z-tokens), has fixed presets.
* [rpass](https://github.com/timkuijsten/rpass)

## License

MIT, see [License](LICENSE.md) file.

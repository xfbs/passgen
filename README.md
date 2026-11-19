# Passgen 

Passgen is a tool to generate random sequences from a [regular
expression](https://en.wikipedia.org/wiki/Regular_expression). It has some
features that make it uniquely useful for generating secure passphrases, such
as supporting choosing words from a wordlist, building a markov-chain from a
wordlist to generate high-entropy, but pronounceable sequences, and being able
to measure the actual entropy of every sequence it generates. 

* Website: [passgen.io](https://passgen.io)
* Documentation: [passgen.io/docs](https://passgen.io/docs/)
* Install: [passgen.io/install](https://passgen.io/install/), with nightly builds
* Releases: [GitHub](https://github.com/xfbs/passgen/releases)
* Developer Documentation: [docs/](docs/README.md)
* Source Documentation: [Doxygen](https://passgen.gitlab.io/passgen/docs/index.html)
* Code Coverage: [lcov](https://passgen.gitlab.io/passgen/coverage/lcov), [llvm-cov](https://passgen.gitlab.io/passgen/coverage/llvm-cov)
* Continuous Integration: [Pipelines](https://gitlab.com/passgen/passgen/pipelines)

## Features

- Generates secure, random sequences using your system's cryptographic number
  generator or a master passphrase.
- Customizable to fit any password policy due to it's use of regular
  expressions.
- Ability to measure the entropy of the generated sequences, to target
  passphrases to any security level.
- Can read wordlists to choose words at random (like
  [diceware](https://diceware.dmuth.org/)), or create high-entropy but
  pronounceable phrases using a markov chain.
- Comes with some presets that offer good security.
- Can be embedded into other applications (as a C shared library) or used
  programmatically (using JSON output).
- Permissively licensed (MIT license).

## Examples

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
$ passgen --preset apple2
MKCvtR-N2YNJP-29KRae

$ passgen --preset firefox
kmRxBcUCnvQNAL4

$ passgen --preset proquint64
zabil-pukug-punud-lasup
```

For any sequence that Passgen generates, it can measure the entropy it took to
generate it, so that you have a reasonable estimation of how difficult it would
be to crack the sequence using a brute-force approach.

```
$ passgen --entropy '[a-c]{3}'
entropy: 4.75 bits
cab

passgen --entropy '[a-zA-Z0-9]{32}'
entropy: 190.53 bits
QjKV1q0DKZ8Pc2GM5ccQbYxXfwAbjECk
```

Passgen supports reading in wordlists. You can use this to generate
diceware-like passphrases, for example. Since it has full Unicode support, it
can read wordlist for any language. Here are some examples:

```
$ passgen --entropy --wordlist english:/usr/share/dict/words '\w{english}(-\w{english}){4}'
entropy: 83.35 bits
limelight-icicles-commoner-stamped-garnet

$ passgen --wordlist german:/usr/share/dict/ngerman '\w{german}(-\w{german}){4}'
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
from a master passphrase. In master passphrase mode, Passgen always generates
the same passphrase for the same master passphrase, domain and user, so you
only need to remember a single master passphrase. Under the hood, it uses
[Argon2](https://en.wikipedia.org/wiki/Argon2) to derive a key from your master
passphrase, and the
[ChaCha20](https://en.wikipedia.org/wiki/Salsa20#ChaCha_variant) stream cipher
as a pseudorandom number generator using the derived key.

A master passphrase is set up once, with `passgen --master-setup`, which asks
a few questions, explaining what they mean, and prints what to add to the
configuration (with a short hash of it, which catches typos). Then it is used
by its name, and asked for:

```
$ passgen --master-setup main >> ~/.config/passgen/config.toml
How hard should it be to crack? Higher is harder, but slower.
  1: fast (64 MiB of memory)
  2: recommended (256 MiB of memory)
  3: strongest (1 GiB of memory)
level [2]:

Use a seed? Without one, the master passphrase alone generates
your passphrases, on any computer. With one, the seed is needed
too: keep it safe, and back it up.
use a seed? [y/N]:

Choose a strong master passphrase. It is not stored, and cannot
be recovered.
master passphrase:
master passphrase again:
$ passgen -m main -d example.com -u username -p apple2
master passphrase:
eINh6m-RhafGJ-JgiKM0
```

Which master passphrase, preset and offset an account uses can be stored as
a login in the configuration, so that only the domain and the user have to be
given:

```toml
[[login]]
domain = "example.com"
user = "username"
preset = "apple2"
```

```
$ passgen -d example.com -u username
master passphrase:
eINh6m-RhafGJ-JgiKM0
```

## Syntax

The syntax of the Passgen format string is similar to that of regular
expressions, but it has some additional features. These are the operations that
Passgen understands:

| Syntax | Explanation |
| --- | --- |
| `[a-z]`, `[a-zA-Z0-9]`, `[abcdefg]` | Range. Choose a letter at random. You can give it ranges (`a-z`) or individual letters. If you want to use the literal dash as a letter, escape it with a backslash (`[a-z\-]`). |
| `(abc\|def)` | Group. Choose one at random: `abc` or `def`. |
| `a{7}`, `[a-z]{4,6}` | Repetition. Appending a `{n}` to any syntax repeats the last thing *n* times. Appending a `{n,m}` to any syntax repeats the last thing a random amount between *n* and *m* times. If you want to repeat a whole sequence of things, put them into a group (in parentheses). |
| `\w{english}` | Word. Picks a random word from the wordlist with the name `english`. You can use the `-w name:/path/to/list` command-line option to load wordlists. You can load multiple wordlists. |
| `\m{english}` | Markov-Chain. Generates a high-entropy, but pronounceable word by building a Markov-Chain of the wordlist `english`. This allows you to make a tradeoff between memorability and strength: words are easier to remember than completely random sequences. The Markov chain lets you generate random sequences of letters that follow the same probability distribution as the words in the wordlist, meaning that they are pronounceable and thus easier to remember. |

This is only an overview, see the
[documentation](https://passgen.io/docs/syntax/) for all of it.

## Building

In general, if you have all dependencies installed, doing the following should
result in a working binary.

    $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    $ cmake --build build
    $ cmake --install build

See [Building](docs/building.md) for more information about building the
project.

You may also download a [release](https://github.com/xfbs/passgen/releases), or
a signed nightly build from the [website](https://passgen.io/install/).

## Prior art

* [pwgen](https://linux.die.net/man/1/pwgen).
* [apg](https://linux.die.net/man/1/apg): generates random passwords, can
  either be generated such that they are pronounceable, or taken from a
  predefined list of symbols.
* [z-tokens](https://github.com/volution/z-tokens), has fixed presets.
* [rpass](https://github.com/timkuijsten/rpass)
* [LessPass](https://www.lesspass.com/#/)
* [passacre](https://github.com/habnabit/passacre)
* [ssh-keydgen](https://github.com/cornfeedhobo/ssh-keydgen)

## License

MIT, see [License](LICENSE.md) file.

# Usage

<figure markdown>
![Passgen usage](media/usage.svg){ width="600px" }
<figcaption>Passgen usage example.</figcaption>
</figure>

There are two basic ways of using the passgen command-line utility. The
simplest usage is by using a preset. Passgen comes pre-loaded with some useful
presets, but you can override them or set your own. You can use the `--list`
flag to see all available presets. This will show you the names and patterns of
every preset that passgen knows about.

```
$ passgen --list
preset firefox [a-zA-Z0-9]{15}
preset edge [A-Za-f0-9:_\-]{15}
preset apple2 ([a-zA-Z0-9]{6}-){2}[a-zA-Z0-9]{6}
preset apple1 ([a-zA-Z0-9]{3}-){3}[a-zA-Z0-9]{3}
preset uuid [0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}
```

To use any of these presets, you can use the `--preset` flag along with the
name of the preset you want to use.

```
$ passgen --preset apple2
KslRKD-N6aSJr-uchi98
```

The other way of using passgen is by specifying a pattern. This lets you
describe the exact shape of sequence that you want passgen to generate for you,
and this is useful if none of the presets do what you want. The pattern is
described in a syntax that is inspired by regular expressions and is better
explained in the [Syntax](syntax.md) section. Here is a few examples:

```
$ passgen abc
abc
$ passgen '[a-z]{12}'
yxzzoxzytihb
```

Passgen supports more funtionality, which can be used with either preset-based
or pattern-based usage. This is enabled using command-line flags. Usually,
every command-line flag comes in a pair: a long version starting with two
dashes, for example `--amount`, and a short version starting with a single
dash, such as `-a`. Passgen also has a good help text and a manual page.

## Amount

By default, passgen will only generate a single sequence when you run it. But
if you use the `--amount` flag, you can tell it to output multiple sequences.
This is useful for example when you want to generate many passphrases at once.

```
$ passgen --amount 5 '[a-z]{8}'
hwwfdxve
bgcgnywa
yxuybxpa
lzdmtkdk
wtmhlzjx
```

## Entropy Calculation

Entropy is a measure of how much randomness goes into creating a sequence. This
is used to describe the security of a password. The higher this number is, the
better. You can ask passgen to calculate the entropy of every password it
generates, which gives you a good idea of how secure your sequences are. The
[Entropy](entropy.md) section describes this in more detail. You should aim to
use passphrases with an entropy higher than 100.

Using the `--entropy` flag, you can ask passgen to compute the entropy of every
sequence it generates. This estimation is very accurate.

```
$ passgen -e '[a-z]{8}'
entropy: 37.603518 bits
qqgovgbhe
$ passgen -e '[a-zA-Z0-9]{18}'
entropy: 107.175534 bits
I5sD0VPbWzaQw5gtBl
```

## JSON Output

If you would like to use the output of passgen as the input for other tools, it
might make sense to use a standardized format rather than plain text. For this
reason, you can ask passgen to output the sequences as JSON. This is useful,
for example for feeding the output into [jq](https://stedolan.github.io/jq/)
for post-processing. To enable this, use the `--json` flag.

```
$ passgen -j -p apple1
[{"output":"Jwm-N1M-Tz2-Bd9"}]
$ passgen -j -e '[a-z]{19}' -a 2 | jq
[
  {
    "output": "unzoigkddixtagqcdqc",
    "entropy": 89.308355
  },
  {
    "output": "oczkdkqooqhpvgwtjpd",
    "entropy": 89.308355
  }
]
```

## Wordlists

It turns out that completely random characters are quite hard to remember,
especially if you're being a good digital citizen and use a different
passphrase for every website. One trick that is quite common is using random
words from a dictionary, because they are more memorable.

You can use the `--wordlist name:/path` flag to tell passgen to load words from
a wordlist, and then reference it in the pattern with `\w{name}`, which will be
replaced with a random word from the list.

```
$ passgen --wordlist english:/usr/share/dict/words "(\w{english}-){3}\w{english}"
colored-Reuters-whitewashed-mine
```

## Markov Chains

Instead of picking real words from a dictionary, passgen can also use the
dictionary to populate a [Markov chain][markov], which is a specialized data
structure that allows it to learn how words in a given language are constructed
by looking at how frequently different character tuples appear, and it can use
that information to construct words that are not real words but close enough to
be pronounceable and thus memorable.

```
$ passgen --wordlist english:/usr/share/dict/words "(\m{english}-){3}\m{english}"
logis-med-musives-interprefinglidits
```

[markov]: https://en.wikipedia.org/wiki/Markov_chain

## Randomness Source

By default, passgen will use the system's secure random number generator. You
can override this and use a different source of randomness using the `--random`
flag, this is explained more in the [Randomness](randomness.md) section. It is not
recommended to using a different source of randomness because the default is
typically the best.

## Configuration

You can customize passgen by writing your own configuration file. On startup,
passgen will read the system configuration file located at `/etc/passgen.conf`,
and your use configuration file at `~/.config/passgen.conf`, if either of them
exist. 

In these configuration files, you can specify pattern presets and wordlists.
The syntax for this file looks like this:

```
preset mypreset [a-z]{8}
preset otherpreset [a-zA-Z0-9]{18}
wordlist english /usr/share/dict/words
wordlist other /usr/share/dict/other
```

If you save this at `~/.config/passgen.conf`, then you can use the presets you
have defined in there just like any built-in presets.

## Help

You can use the `--help` flag for passgen to get a quick help text with an
overview of all of the flags. Passgen also comes with a good man page, if you
have it installed you can view it like this:

```
$ man passgen
```

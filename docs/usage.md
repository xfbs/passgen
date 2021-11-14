# Usage

The passgen command-line utility accepts a string that has a syntax similar to a regular expression that determines the shape of the password it will generate. For example, if you supply a string like `[a-z]{16}`, passgen will spit out sixteen random letters, from a to z:

```
$ passgen "[a-z]{16}"
plxlrjhkplultuyg
```

With this, you can create extremely secure passwords matching any weird password policy your employer or some website might throw at you. Do they only accept passwords up to 12 characters, and you want to maximize the entropy you can pack into it?

```
$ passgen "[a-zA-Z0-9-_=+!@#$%^&*()]{12}"
*&B%KOxxnBI]
```

Or maybe you want a very secure, long password with dashes but you don't want to have too many crazy symbols in it?

```
$ passgen "([a-z0-9]{8}-){3}[a-z0-9]{8}"
n7k7r9tw-9owbqwm5-89f9fkhw-xnnnxk8q
```

Most of the basic syntax that you are used to from regular expression will work in passgen, too.

```
$ passgen "(this|that) house"
that house
```

## Wordlist-based generation

There is a few more cool things it can do, tho. It turns out that these completely random characters are quite hard to remember, especially if you're being a good digital citizen and use a different password for every website. One trick that is quite common is using random words from a dictionary. You have to use longer passwords to make this safe, but they are significantly easier to memorize. Passgen supports loading in arbitrary wordlists, and has a syntax that you can use to pick one (or more) random words from that list.

```
$ passgen --wordlist english:/usr/share/dict/words "(\w{english}-){3}\w{english}"
colored-Reuters-whitewashed-mine
```

But it can do even better than that. Instead of picking real words from a dictionary, passgen can also use the dictionary to populare a markov chain, which is a specialized data structure that allows it to learn how words in a given language are constructed, and it can use that information to construct words that are not real words in the english language, but are close enough to be pronounceable and thus memoryable.

```
$ passgen --wordlist english:/usr/share/dict/words "(\m{english}-){3}\m{english}"
logis-med-musives-interprefinglidits
```

## Presets

Passgen has a number of presets built-in that you can use out-of-the-box. Using the `-p` option, along with the name of a preset.

```
$ passgen -p apple2
L6stQv-Z4duMn-azVKvU
```

## Entropy estimation

Finally, passgen is able to tell you exactly how much entropy a generated password contains. You can use this information to know exactly how secure a password is. When using passgen to generate passwords for security-critical applications, this information can be used to specify exactly what the minimum security requirements are for a password rather than specifying minimum lengths or characters.

```
$ passgen -c "[a-zA-Z0-9]{16}"
entropy: 95.267141 bit
uoVbYPLeuoR7dCzE
```

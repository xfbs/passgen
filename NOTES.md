# Notes

## Version

When bumping the version, be sure to change it in all places:

* Makefile
* man/passgen.1
* Code

## Generating documentation

The documentation is published in the `gh-pages` branch. Use this workflow to
update it:

    $ git worktree add docs/html gh-pages
    $ make docs
    $ cd docs/html
    $ git add .
    $ git commit -m "updates docs"
    $ git push origin gh-pages
    $ cd ../../

To undo the worktree, do (from the main project dir)

    $ git worktree remove docs/html
    $ make clean

## Generating debian package

Experimental. There's a tutorial for it [here](http://www.tldp.org/HOWTO/html_single/Debian-Binary-Package-Building-HOWTO/). Also see [this](https://www.debian.org/doc/debian-policy/ch-source.html#debian-changelog-debian-changelog).

## Todo

### Unicode Support

To support generating passwords in truly any language, the pattern parser needs
to be adjusted to understand unicode. For this, libraries for parsing and
emitting UTF-8 are necessary.

Use utf8proc as support library for this. The `utf8proc_iterate()` function can
be used to parse utf8 inside the `parse_char()` function, the data structures
need to be patched to use `uint32_t` instead of chars, and the
`utf8proc_encode_char()` needs to be used in the random pattern generation
functions to write out utf8.

### [DONE] Range parsing

When parsing `[c-a]`, ~rewrite it to `a-c`~ cause an error and stop parsing.

### Implement error handling

Right now, parsing functions return `NULL` both when they encounter an error
and when there is nothing left to parse. This is suboptimal. They should also
return a proper error, maybe some kind of `parse\_error` that includes not just
the error kind but also some context like the position?

### Linear parsing

Intead of parsing into a tree-like structure with linked lists, allow
parsing into a linear buffer. Not really necessary.

### Add return wrapper type for parsing

How about a `parse\_result` type with an okay field and an optional error?

### Abstract reading

Add a `read\_type` struct that is generic over where to read from (such that
UTF-8 support can be made optional)

### Using getrandom

~Reading from `/dev/urandom` could be replaced with the `getrandom()`
syscall.~

Alternative idea: use `libsodium` randomness functions for operating system
independence.

### Word List

Supporting the `\w` special character, along with a wordlist, should enable
[XKCD-936](https://www.xkcd.com/936/)-compliant passwords. This would
complicate the random generation code a bit in order to allow something like
`\w{5,8}` for a random word of length 5—8. Also, it means that passgen would
likely need to bundle some kind of wordlist.

### Pronounceable character strings

Supporting some kind of mechanism to generated pronounceable passwords should
be implemented.

### Supporting escape codes

Supporting escape codes like `\n`, `\t`, `\uAAFF`, `\xAF`, etc. should be
implemented. This needs to be done in the `parse\_char` method, most likely.

### Migrate pattern\_parse into pattern\_group\_parse

That makes more sense, and it allows pattern\_parse to do some error
checking/handling.

### Implement probability

For a range `[abc]`, implement something like `[a{9}bc]` to make `a` have 9
times the relative probability of the whole thing. This allows one to adjust
how often which items should be picked with a similar format to how lengths are
specified.

### Implement custom randomness source

Use a custom random data stream (or a key file that is hashed and the hash used
to seed a PRNG?)

Idea: use `libsodium`'s stream ciphers, such as ChaCha20, XChaCha20, Salsa20 or
XSalsa20, along with a secret key or keyfile, to generate reproducible passes.

### Add disclaimer about timing sidechannels

When generating variable-length passwords, the length is a sidechannel.

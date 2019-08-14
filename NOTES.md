# Notes

## Building

Use the Makefile to build the project. For example, to build it in release
mode, do:

    $ make release

When compiling, resulting files are placed in `./build/`. There are multiple
targets:

* `release` for a release build with optimisations enabled.
* `debug` for a debug build without optimisations, with debug symbols and
  `DEBUG` defined.
* `debug-address` for a debug build with LLVM AddressSanitizer.
* `debug-memory` for a debug build with LLVM MemorySanitizer.
* `debug-undefined` for a debug build with LLVM UndefinedSanitizer.

## Actions

* `make format` to use clang-format to clean up the code.
* `make clean` to remove all build output.
* `make test` build and run tests
* `make bench` build and run benchmarks.
* `make docs` build docs using doxygen.

## Tools

This repository also contains a few tools that are useful in working with or
inspecting the parsed data.

* `pattern-dump` parses a pattern and dumps it in a human-readable format.
* `pattern-choices` calculates how many possible choices there are for a given
  pattern.

## Version

When bumping the version, be sure to change it in the `meson.build` file, which
contains the autoratative version number that is used to generate the man page
and the code.

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

UTF8 iterator parsing has already been implemented, what is still needed is to
bake this API into the code. Also, emitting UTF8 has yet to be done.

### [DONE] Range parsing

When parsing `[c-a]`, ~rewrite it to `a-c`~ cause an error and stop parsing.

### Implement error handling

Right now, parsing functions return `NULL` both when they encounter an error
and when there is nothing left to parse. This is suboptimal. They should also
return a proper error, maybe some kind of `parse\_error` that includes not just
the error kind but also some context like the position?

### Linear parsing

~Intead of parsing into a tree-like structure with linked lists, allow
parsing into a linear buffer.~ Not really necessary.

Instead, maybe some of the code could be changed from using linked lists to
using an array.

### Add return wrapper type for parsing

How about a `parse\_result` type with an okay field and an optional error?

### Abstract reading

~Add a `read\_type` struct that is generic over where to read from (such that
UTF-8 support can be made optional)~ overcomplicated, not really needed.

### Using getrandom

~Reading from `/dev/urandom` could be replaced with the `getrandom()`
syscall.~

Alternative idea: use `libsodium` randomness functions for operating system
independence.

### Implement custom randomness source

Use a custom random data stream (or a key file that is hashed and the hash used
to seed a PRNG?)

Idea: use `libsodium`'s stream ciphers, such as ChaCha20, XChaCha20, Salsa20 or
XSalsa20, along with a secret key or keyfile, to generate reproducible passes.

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

### Add disclaimer about timing sidechannels

When generating variable-length passwords, the length is a sidechannel.

### Write proper tokenizer for the parser.

Use a buffered tokenizer with peek().

This should be made to handle:

- escaped chars `\\, \n, \r, \(, \[`
- raw unicode chars `\u{a4}`
- regex special chars `\w, \s`

### Store offsets in the parsed structure

When parsign something, always store the offset in the source string.  That way
we can used the parsed data structure for other things, such as syntax
highlighting.

### Generate characters from custom languages

Support something like `\l{English}` or `\l{German}` to generate words from
that language or script.

### Implement 

[RFC972](https://tools.ietf.org/html/rfc972) is about password generation.
Maybe there are more RFCs that are relevant.

### Security features

`mprotect` to avoid passwords being swapped out? seccomp or similar mechanisms
to add protection?

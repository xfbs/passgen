# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- `\]` and `\[` to have brackets in a set.
- `passgen_allocator_set()` replaces the allocator which the library uses.
- `passgen/passgen.h` includes all of the public headers.
- A pkg-config file, `passgen.pc`.
- `passgen_random_desc_secret()` says whether a description of a source of
  randomness may hold a secret, so that it is not shown.
- The public headers can be used from C++.
- The API documentation has a main page with the conventions of the library,
  and an example which is built and run as a test (`examples/`).

### Changed
- Errors in patterns say what is wrong, rather than "invalid token", and mark
  all of what is wrong.
- `passgen_parse()` returns a `passgen_status`. `passgen_error` has the kind of
  error, where it is, and what it was caused by.
- The functions for generating return a `passgen_status`, take an error to
  fill in, and return the length through a pointer. Failing to generate says
  why, such as which wordlist is missing.
- A wordlist without a name (`\w{}`) is not a valid pattern.
- A wordlist which cannot be read says why. One which could only be read in
  part is an error, rather than being used with the words that were read.
- `passgen_env_wordlist_add()` returns a `passgen_status`.
- Running out of memory while parsing a pattern is an error
  (`PASSGEN_ERROR_OUT_OF_MEMORY`), rather than a crash.
- Running out of memory while reading a wordlist or building its Markov chain
  is an error, rather than a crash.
- A source of randomness which runs out is an error (`PASSGEN_ERROR_RANDOM`,
  `passgen_random_failed()`), rather than aborting.
- Entropy is in bits in the API, rather than a number of possibilities.
- Empty sets (`[]`) are rejected.
- Empty groups such as `()` are kept rather than removed, and use randomness
  like other groups, as the reference implementation does. What patterns with
  empty groups generate changes, also in master passphrase mode.
- A group in which every choice has a multiplier of zero, such as `x({0}a)`,
  fails to generate (`PASSGEN_ERROR_GROUP_EMPTY`), as it does for the whole
  pattern, rather than being left out.
- `passgen_random_chacha20_argon2_open()` takes `passgen_argon2_params`.
- `passgen_random_chacha20_seek()` returns whether it could seek, and leaves
  other sources alone, rather than corrupting them in release builds.
- A `max` of zero for `passgen_random_u*_max()` aborts in every build, rather
  than looping forever in release builds.
- `passgen_pattern` is opaque. `passgen_parse()` allocates it, and
  `passgen_pattern_free()` frees it.
- `passgen_env` is opaque, and created with `passgen_env_new()`. Its header is
  `passgen/env.h`.
- The environment no longer frees the source of randomness, and copies the
  names of wordlists.
- Only the public headers are installed: `passgen.h`, `generate.h`, `env.h`,
  `random.h`, `error.h`, `allocator.h`, `version.h`, `config.h` and
  `export.h`.
- The shared library only exports the public interface.
- The command-line tool is linked against the static library, and only uses
  the public interface.
- `--list` shows the presets sorted by name.
- Releases show the commit they were built from in `--version`.
- Releases and continuous integration use Alpine 3.24 and Debian 12.
- Nightly builds are named `passgen-nightly-*`, and their packages have a
  version like `0.2.0+nightly20261117`.
- The documentation for users is on passgen.io, and no longer in `docs/`.
- A dash in a set is only ever part of a range. To have a dash itself, escape
  it (`[a-z\-]`). A dash at the start or the end of a set (`[-a]`, `[a-]`) or
  after a range (`[a-z0-9-_]`) is an error. A dash after a range used to
  change the end of that range, so that `[a-zA-Z0-9-_]` meant `0` to `_`
  rather than a dash and an underscore. Passwords derived from such a pattern
  are not the same with the fixed pattern.
- A repetition or a multiplier without a number (`a{}`, `a{,}`, `{}a`) is an
  error, rather than zero. The minimum can still be left out (`a{,3}`).
- The characters of a set are sorted, so that sets with the same characters
  generate the same (`[ba]` and `[ab]`, or `[a-zA-Z0-9]` and `[0-9A-Za-z]`).
  This changes what most patterns with a set generate from a deterministic
  source of randomness, including every password derived from a master
  passphrase with such a pattern, and with the presets `apple1`, `apple2`,
  `edge` and `firefox`.
- The key and IV of the `chacha20` source of randomness are in hex: exactly 64
  hex digits for the key, and exactly 16 for the IV (if there is one). They
  used to be taken as bytes, padded with zero bytes, and an IV which was too
  long was cut off. To keep using an old key, write its bytes in hex and pad
  it with zeroes: `chacha20:key` is
  `chacha20:6b6579` followed by 58 zeroes. An empty IV (`chacha20:key:`) is
  16 zeroes.

### Removed
- `passgen_env_init()` and `passgen_env_config()`.
- `PASSGEN_MLOCK` from `passgen/config.h`, as it is only for the
  command-line tool.
- The `PASSGEN_MEMORY` build option, which did nothing.
- Nightly builds for macOS and Windows. Releases have a build for macOS.

### Fixed
- `config.h.in` being installed along with the headers.
- Ranges ending in an escaped character (`[+-\-]`) failing to generate.
- Entropy above 1024 bits being reported as infinite.
- The API documentation leaving out the ChaCha20 sources of randomness.
- The library printing to stderr when `/dev/urandom` cannot be opened, on
  systems without `getrandom()` or `arc4random()`.
- A repetition or question mark following an empty group applying to what
  comes before the group: `a(){3}` generated `aaa`.
- Building with `-DPASSGEN_MONOCYPHER=OFF`. Such a build does not have the
  options for master passphrase mode.
- `passgen_env_wordlist_add()` crashing in release builds when given a length
  of the Markov chain of zero or more than 255. It returns the new
  `PASSGEN_ERROR_INVALID_ARGUMENT` instead.
- An alternative with a multiplier of zero making the group choose wrongly:
  `({0}a|b)` generated nothing, and `({0}a|b|c)` always chose `b`. Passwords
  derived from such a pattern change.
- The entropy of a set counting a character which is listed more than once
  (`[aab]`, `[a-zc]`) as a different character every time, which overstated
  it. It now depends on how many times the chosen character was listed.
- A randomness source which cannot be used printing all of its description in
  the error, including a key or a master passphrase. Only the kind is printed
  now, except for sources which cannot hold a secret (`file`, `xorshift`).
- Builds with seccomp (`PASSGEN_SECCOMP`) being killed when the output is not
  a terminal or a pipe (such as `passgen a > /dev/null`), with glibc 2.41 and
  later (which implements `getrandom()` in the vDSO), when deriving a key with
  transparent huge pages enabled, and with musl on architectures which only
  have `openat` (such as aarch64). They also did not compile with GCC 14 and
  later.

## [0.2.0] - 2026-11-17
### Added
- GitHub Actions workflows for CI and automated releases.
- CTest integration and `check` target.
- Regression tests and more CLI tests.
- Test vectors for the deterministic randomness sources, with a reference
  implementation to generate them.
- Fuzzing harnesses, with a script and `fuzz` target to run them.
- Version macros in `passgen/config.h`.
- `\?` to have a question mark in the output.

### Changed
- Master passphrase mode uses stronger Argon2 parameters and a longer salt.
- Output of the deterministic randomness sources has changed. This includes
  every passphrase derived from a master passphrase.
- Output is no longer limited to 256 bytes.
- Failing to generate is an error instead of an empty line.
- Errors are printed to standard error.
- User configuration is read from `$XDG_CONFIG_HOME`.
- Optional items (`a?`) count towards entropy.
- Repetitions like `a{5,2}` are rejected.
- `--master` cannot be combined with `--random`, and `--domain` and `--token`
  need `--master`.
- Unicode escapes above `\u{10ffff}` are rejected.
- UTF-8 which ends in the middle of a sequence is rejected.
- Locking memory is best-effort, and no longer makes large allocations fail.
- `passgen_random_chacha20_argon2_open()` takes strings.
- The shared library is versioned (`libpassgen.so.0.2`).
- `passgen_version_compatible()` requires the same minor version before 1.0.0.
- The man page is up to date.
- Requires CMake 3.16.

### Fixed
- Random bytes reused at the buffer boundary.
- Half of the randomness wasted for choices that are a power of two.
- Large `xorshift:` seeds all resulting in the same output.
- Stack overflows parsing `chacha20:` and `chacha20-argon2:` descriptions.
- Stack overflow generating long words from a Markov chain.
- Exhausted randomness sources going unnoticed in release builds.
- Infinite loop with empty wordlists.
- Crash for wordlists without a name (`\w{}`).
- Crash when a segment has more than one multiplier (`{4}{1}a`).
- Out of bounds write for invalid unicode escapes in wordlist names.
- Characters such as `ż` being mistaken for pattern syntax.
- Configuration examples using the wrong syntax.
- Buffer overflows loading the configuration.
- `--depth` having no effect.
- Unescaped control characters in JSON output.
- Memory leaks in the CLI.
- Stray files installed into `share/passgen`.

## 0.1.2 - 2019-08-18
### Added
- Benchmarking tool.

### Fixed
- Installing the passgen binary.

## [0.1.1] - 2019-08-14
### Added
- Revamped testing system, now uses autogeneration.
- Unicode iterator API.
- Fixed installing.
- Nix shell config for easier testing.

## [0.1.0] - 2019-07-14
### Added
- Initial release of project.

[Unreleased]: https://github.com/xfbs/passgen/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/xfbs/passgen/releases/tag/v0.2.0
[0.1.1]: https://github.com/xfbs/passgen/releases/tag/v0.1.1
[0.1.0]: https://github.com/xfbs/passgen/releases/tag/v0.1.0

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Passwords which change

- A wordlist with Windows line endings (`\r\n`) no longer has the carriage
  return at the end of every word, which changes what is generated from it.
- A pattern whose last segment has a weight of zero, such as `a|{0}b`, no
  longer uses randomness to choose between its segments when there is only
  one left, as it already did in a group, such as `(a|{0}b)`.

### Added

- Markov chains of length zero, as in `\m{name:0}`, which choose every
  character by how often it is in the words alone.

### Changed

- A segment with more than one weight, such as `({2}{3}a|b)`, is an error.
  The last one used to count.
- A group with nothing to choose from, such as `({0}a)`, is an error when the
  pattern is parsed, also when it is repeated zero times. It used to be one
  only when it was generated.
- A wordlist with a word which is not valid UTF-8 is an error when it is
  read, which says the line. It used to be one only when the word was chosen
  (and the word was left out of Markov chains).
- Groups can be nested 256 deep, also through presets, rather than 1024.
- Errors only have colors when they are printed to a terminal, and not when
  `NO_COLOR` is set.

## [0.5.0] - 2026-11-20

### Passwords which change

All passwords derived from a master passphrase change. Master passphrase mode
derives a key from the master passphrase once, with more effort, and from it
a key for the domain and the user. To get an old password back, use 0.4.0.

With `--amount`, every sequence after the first one changes for deterministic
sources of randomness: each is generated from its own stream, so that it does
not depend on the ones before it.

### Added

- `--config` (`-c`), to read a configuration file instead of the usual ones,
  and `PASSGEN_CONFIG`, a list of them.
- Master passphrases are set up in the configuration, as `[master.NAME]`, with
  `--master-setup [NAME]`, which asks how. They have a level (how much effort
  deriving the key takes), a short hash, which catches a mistyped master
  passphrase, and optionally a seed. `--master-level`, `--master-seed` and
  `--master-version` set these without asking.
- `--offset N`, to generate the Nth sequence of a deterministic source of
  randomness (counting from 0), such as to change a password in master
  passphrase mode.
- A wordlist in the configuration can have the hash of its file, as
  `{ path = "...", hash = "..." }`, which it then has to have. The hash is
  what `b2sum` prints, or any start of it. The EFF wordlists which are
  installed are checked this way.
- Logins in the configuration, as `[[login]]`, which say which master
  passphrase, preset or pattern and offset a domain and user use, so that
  `passgen -d DOMAIN -u USER` is enough.

### Changed

- The configuration is in TOML, with the tables `wordlists` and `presets`, and
  a relative path of a wordlist is relative to the file. It is read from
  `/usr/share/passgen/config.toml` (which is installed), then
  `/etc/passgen/config.toml`, then `~/.config/passgen/config.toml`, each of
  which can replace or remove (with `false`) what the ones before define.
- The source of randomness can no longer be set in the configuration.
- `--master` (`-m`) takes the name of a master passphrase from the
  configuration, and asks for the passphrase, rather than taking it as an
  argument.
- `--token` (`-t`) is now `--user` (`-u`).
- The `chacha20-argon2` source of randomness is now
  `master:VERSION:LEVEL:PASSPHRASE:SEED:DOMAIN:USER`, where colons and
  backslashes are escaped with a backslash.
- Problems with a master passphrase are errors with the exit status 15.
- Problems with the configuration are errors, with the exit status 14.
- A wordlist is only opened when a pattern uses it, so one which does not
  exist is only an error then, which says where it is.

### Fixed

- Builds with seccomp being killed when reading from a character device, such
  as with `-r file:/dev/urandom`.

## [0.4.0] - 2026-11-19

### Passwords which change

All passwords derived from a master passphrase (or from any deterministic
source of randomness) change, also for the presets. Choices with only one
option no longer use randomness: groups with a single alternative (`(abc)`,
and every pattern itself), sets of a single character (`[a]`), wordlists with
a single word, and letters of Markov chains which only one letter can follow.
So `(abc)` generates the same as `abc`. Markov chains also choose between
letters in a different order. To get an old password back, use 0.3.0.

### Added

- `-S`/`--steps` to limit how long generating takes. The default of a million
  steps makes patterns such as `(){999999999}` fail rather than run
  practically forever.
- `\p{name}` uses a preset in a pattern, also in other presets, so that
  patterns can be built from parts. It used to generate nothing.
- Presets whose names start with an underscore are not shown by `--list`, for
  parts of other presets.
- `\m{name:2}` uses a Markov chain of that length (from 1 to 8, 3 without
  one). A pattern can use different lengths from the same wordlist.

### Changed

- Problems with wordlists (missing, empty or unreadable) are found before
  anything is generated, also when the pattern would not have used the
  wordlist, and show where in the pattern it is used.
- Markov chains need less memory, up to half as much for long ones, and much
  less for wordlists in scripts with many letters.
- `--help` shows all of the pattern syntax, and the sources of randomness.
- The library interface changed incompatibly.

### Removed

- `-D`/`--depth`. Groups can be nested up to 1024 deep, which is now checked
  when parsing.
- `-M`/`--markov-length`, which is part of the pattern now: `\m{name:2}`.

### Fixed

- A crash after the error for patterns with groups nested very deeply.
- `--entropy` showing the entropy after the output it is for, rather than
  before it, on a terminal.

## [0.3.0] - 2026-11-18

### Passwords which change

Passwords derived from a master passphrase (or from any deterministic source
of randomness) change for patterns with:

- sets whose characters are not in order, such as `[a-zA-Z0-9]`, and so the
  presets `apple1`, `apple2`, `edge` and `firefox`
- empty groups, such as `a(){3}`
- alternatives with a multiplier of zero, such as `({0}a|b|c)`

Some patterns which used to generate are errors now (see below). To get an old
password back, use 0.2.0.

### Added

- `\[` and `\]` in sets.
- RPM packages.

### Changed

- Sets are sorted, so `[ba]` generates the same as `[ab]`.
- A dash in a set has to be escaped (`[a-z\-]`) unless it is part of a range.
  `[a-z0-9-_]` used to mean `0` to `_`.
- `a{}`, `{}a`, `[]` and groups whose choices all have a multiplier of zero
  (`x({0}a)`) are errors.
- Empty groups are kept, and use randomness like other groups.
- `chacha20:` takes its key and IV in hex (64 and 16 digits). The old
  `chacha20:key` is `chacha20:6b6579` followed by 58 zeroes.
- Errors in patterns say what is wrong, and where.
- Wordlists which cannot be read completely, and sources of randomness which
  run out (such as a short `file:`), are errors.
- `--list` is sorted, and `--version` shows the commit.
- Releases include a statically linked command-line tool, and archives and a
  Debian package with the library, also for macOS. The statically linked tool
  is built on Alpine 3.24, the rest for Linux on Ubuntu 22.04, and the RPM
  packages on AlmaLinux 9.
- Nightly builds are named `passgen-nightly-*`.
- The documentation is on <https://passgen.io>.
- The library interface changed incompatibly.

### Removed

- Nightly builds for macOS and Windows.

### Fixed

- Sets with ranges ending in an escaped character (`[+-\-]`) failing to
  generate.
- `a(){3}` generating `aaa`.
- `({0}a|b)` generating nothing, and `({0}a|b|c)` never choosing `c`.
- Entropy being overstated for sets which list a character more than once
  (`[aab]`), and shown as infinite above 1024 bits.
- Errors about the source of randomness showing keys and master passphrases.
- Builds with seccomp crashing, such as when writing to `/dev/null`.

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

[Unreleased]: https://github.com/xfbs/passgen/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/xfbs/passgen/releases/tag/v0.5.0
[0.4.0]: https://github.com/xfbs/passgen/releases/tag/v0.4.0
[0.3.0]: https://github.com/xfbs/passgen/releases/tag/v0.3.0
[0.2.0]: https://github.com/xfbs/passgen/releases/tag/v0.2.0
[0.1.1]: https://github.com/xfbs/passgen/releases/tag/v0.1.1
[0.1.0]: https://github.com/xfbs/passgen/releases/tag/v0.1.0

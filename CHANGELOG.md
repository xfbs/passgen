# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

### Removed

- `-D`/`--depth`. Groups can be nested up to 1024 deep, which is now checked
  when parsing.
- `-M`/`--markov-length`, which is part of the pattern now: `\m{name:2}`.

### Fixed

- A crash after the error for patterns with groups nested very deeply.

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

[Unreleased]: https://github.com/xfbs/passgen/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/xfbs/passgen/releases/tag/v0.3.0
[0.2.0]: https://github.com/xfbs/passgen/releases/tag/v0.2.0
[0.1.1]: https://github.com/xfbs/passgen/releases/tag/v0.1.1
[0.1.0]: https://github.com/xfbs/passgen/releases/tag/v0.1.0

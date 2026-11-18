# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- `\]` and `\[` to have brackets in a set.
- `passgen_allocator_set()` replaces the allocator which the library uses.

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
- `passgen_random_chacha20_argon2_open()` takes `passgen_argon2_params`.
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
- The command-line tool is linked against the static library.
- Releases show the commit they were built from in `--version`.
- Nightly builds are named `passgen-nightly-*`, and their packages have a
  version like `0.2.0+nightly20261117`.
- The documentation for users is on passgen.io, and no longer in `docs/`.

### Removed
- `passgen_env_init()` and `passgen_env_config()`.
- The `PASSGEN_MEMORY` build option, which did nothing.
- Nightly builds for macOS and Windows. Releases have a build for macOS.

### Fixed
- `config.h.in` being installed along with the headers.
- Ranges ending in an escaped character (`[+-\-]`) failing to generate.
- Entropy above 1024 bits being reported as infinite.

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

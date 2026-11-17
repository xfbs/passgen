# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- GitHub Actions workflows for CI and automated releases.
- CTest integration and `check` target.
- Regression tests and more CLI tests.

### Changed
- Output is no longer limited to 256 bytes.
- Failing to generate is an error instead of an empty line.
- Errors are printed to standard error.
- User configuration is read from `$XDG_CONFIG_HOME`.
- Optional items (`a?`) count towards entropy.
- Repetitions like `a{5,2}` are rejected.
- Requires CMake 3.16.

### Fixed
- Random bytes reused at the buffer boundary. This changes deterministic
  output (including master passphrase mode) past 1024 bytes of randomness.
- Stack overflows parsing `chacha20:` and `chacha20-argon2:` descriptions.
- Stack overflow generating long words from a Markov chain.
- Exhausted randomness sources going unnoticed in release builds.
- Infinite loop with empty wordlists.
- Buffer overflows loading the configuration.
- `--depth` having no effect.
- Unescaped control characters in JSON output.
- Stray files installed into `share/passgen`.

## [0.1.2] - 2019-08-18
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

[Unreleased]: https://github.com/xfbs/passgen/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/xfbs/passgen/releases/tag/v0.1.0
[0.1.1]: https://github.com/xfbs/passgen/releases/tag/v0.1.1

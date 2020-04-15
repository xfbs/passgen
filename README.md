# Passgen [![Build Status](https://travis-ci.org/xfbs/passgen.svg?branch=master)](https://travis-ci.org/xfbs/passgen) ![GitHub tag (latest SemVer)](https://img.shields.io/github/tag/xfbs/passgen.svg)

Passgen is a tool to generate passwords from a regex-like format string.

## Usage

```
passgen version 0.1.0
Generate passwords from a regex-like pattern.
Usage: ./build/release/passgen [OPTIONS] [PATTERN]

PATTERN is a regex-like string describing the password.
  abc|def            Matches string 'abc' or 'def' (choice).
  [a-cf]             Matches character 'a', 'b', 'c', and 'f' (range).
  (abc)              Matches strings 'abc' (group).
  [a-c]{2,3}         Matches between 2 and 3 repetition of element (repeat).

OPTIONS
  -a, --amount       The amount of passwords
  -h, --help         Show this help information
  -p, --preset name  Use the given preset.
  -v, --version      Show the version of this build.

PRESETS
  apple1             Generate passwords like 'oKC-T37-Dew-Qyn'.
  apple2             Generate passwords like 'mHXr4X-CiK4w6-hbjF7T'.
```

## Examples

Create 12-digit alphanumeric password.

    $ passgen "[a-zA-Z0-9]{12}"
    DnJ4bOIGmZbj

## Installing

If you are on macOS, you can install this using homebrew.

    brew install xfbs/local/passgen

## Building

See the [BUILDING.md](BUILDING.md) for more information about building the project.

## Developers

Code Coverage: [lcov](https://xfbs.gitlab.io/passgen/coverage/lcov), [llvm-cov](https://xfbs.gitlab.io/passgen/coverage/llvm-cov).  
Pipelines: https://gitlab.com/xfbs/passgen/pipelines  

## Prior art

* [pwgen](https://linux.die.net/man/1/pwgen).
* [apg](https://linux.die.net/man/1/apg).

## License

MIT, see [License](LICENSE) file.

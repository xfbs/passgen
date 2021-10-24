# Passgen 

Passgen is a tool to generate passwords from a regex-like format string.

Code Coverage: [lcov](https://xfbs.gitlab.io/passgen/coverage/lcov), [llvm-cov](https://xfbs.gitlab.io/passgen/coverage/llvm-cov).  
Documentation: [![Docs](https://xfbs.gitlab.io/passgen/badges/docs.svg)](https://xfbs.gitlab.io/passgen/doxygen/index.html).  
Continuous Integration: [![GitLab Pipelines](https://gitlab.com/xfbs/passgen/badges/master/pipeline.svg)](https://gitlab.com/xfbs/passgen/pipelines).  
Snapshots:
  - [passgen-master-amd64.tar.xz](https://xfbs.gitlab.io/passgen/snapshot/passgen-master-amd64.tar.xz),
  - [passgen-master-musl-amd64.tar.xz](https://xfbs.gitlab.io/passgen/snapshot/passgen-master-musl-amd64.tar.xz),
  - [passgen-master-osx-amd64.tar.xz](https://xfbs.gitlab.io/passgen/snapshot/passgen-master-osx-amd64.tar.xz).

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

Create something that looks like an email address.

    $ passgen '[a-z0-9.]{3,10}@[a-z]{3,10}.(com|net|org)'
    njd0xh@gbnjzlkij.com

Create an password consisting of three pronounceable (but randomly generated) words.

    $ passgen '\p[english]{9,12}(-\p[english]{9,12}){2}'
    arapurity-readygoauce-nathundly

## Installing

If you are on macOS, you can install this using homebrew.

    brew install xfbs/local/passgen

## Building

In general, if you have all dependencies installed, doing the following should result in a working binary.

    $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    $ cmake --build build
    $ cmake --install build

See the [BUILDING.md](BUILDING.md) for more information about building the project.

## Prior art

* [pwgen](https://linux.die.net/man/1/pwgen).
* [apg](https://linux.die.net/man/1/apg): generates random passwords, can either be generated such that they are pronounceable, or taken from a predefined list of symbols.

## License

MIT, see [License](LICENSE) file.

There are word lists included in this repository, their respective license applies:

- German word list taken from <https://github.com/davidak/wortliste>
- Latin word list taken from <https://github.com/bbloomf/verbalatina>

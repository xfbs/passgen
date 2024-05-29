# Passgen [![CI Pipelines](https://gitlab.com/xfbs/passgen/badges/master/pipeline.svg)](https://gitlab.com/xfbs/passgen/-/pipelines) [![Documentation](https://xfbs.gitlab.io/passgen/badges/docs.svg)](https://passgen.it/docs/index.html)

Passgen is a tool to generate passwords from a regex-like format string.

* Website: [passgen.it](https://passgen.it)
* Source Documentation: [Doxygen](https://passgen.it/docs/index.html)
* Code Coverage: [lcov](https://passgen.it/coverage/lcov), [llvm-cov](https://passgen.it/coverage/llvm-cov)
* Continuous Integration: [Pipelines](https://gitlab.com/xfbs/passgen/pipelines)
* Releases: [Releases](https://passgen.it/releases)

## Usage

![Passgen usage](https://passgen.it/media/usage.svg)

## Examples

Create 12-digit alphanumeric password.

    $ passgen "[a-zA-Z0-9]{12}"
    DnJ4bOIGmZbj

Create something that looks like an email address.

    $ passgen '[a-z0-9.]{3,10}@[a-z]{3,10}.(com|net|org)'
    njd0xh@gbnjzlkij.com

Create an password consisting of three pronounceable (but randomly generated) words.

    $ passgen --wordlist english:/usr/share/dict/british-english '\p[english]{9,12}(-\p[english]{9,12}){2}'
    arapurity-readygoauce-nathundly

## Building

In general, if you have all dependencies installed, doing the following should result in a working binary.

    $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    $ cmake --build build
    $ cmake --install build

See [Building](https://passgen.it/development/building) for more information about building the project.

## Prior art

* [pwgen](https://linux.die.net/man/1/pwgen).
* [apg](https://linux.die.net/man/1/apg): generates random passwords, can either be generated such that they are pronounceable, or taken from a predefined list of symbols.
* [z-tokens](https://github.com/volution/z-tokens), has fixed presets.
* [rpass](https://github.com/timkuijsten/rpass)

## License

MIT, see [License](LICENSE.md) file.

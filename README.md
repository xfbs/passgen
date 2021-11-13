# Passgen 

Passgen is a tool to generate passwords from a regex-like format string.

Code Coverage: [lcov](https://xfbs.gitlab.io/passgen/coverage/lcov), [llvm-cov](https://xfbs.gitlab.io/passgen/coverage/llvm-cov).  
Documentation: [![Docs](https://xfbs.gitlab.io/passgen/badges/docs.svg)](https://xfbs.gitlab.io/passgen/doxygen/index.html).  
Continuous Integration: [![GitLab Pipelines](https://gitlab.com/xfbs/passgen/badges/master/pipeline.svg)](https://gitlab.com/xfbs/passgen/pipelines).  

Nightly builds from the master branch:
  - [passgen-linux-amd64.tar.xz][] ([signature][passgen-linux-amd64.tar.xz.sig])
  - [passgen-linux-amd64.deb][] ([signature][passgen-linux-amd64.deb.sig])
  - [passgen-linux-musl-amd64.tar.xz][] ([signature][passgen-linux-musl-amd64.tar.xz.sig])
  - [passgen-linux-aarch64.tar.xz][] ([signature][passgen-linux-aarch64.tar.xz.sig])
  - [passgen-linux-aarch64.deb][] ([signature][passgen-linux-aarch64.deb.sig])
  - [passgen-linux-riscv64.tar.xz][] ([signature][passgen-linux-riscv64.tar.xz.sig])
  - [passgen-linux-riscv64.deb][] ([signature][passgen-linux-riscv64.deb.sig])
  - [passgen-macos-amd64.tar.xz][] ([signature][passgen-macos-amd64.tar.xz.sig])
  - [passgen-win64-amd64.zip][] ([signature][passgen-win64-amd64.zip.sig])

To verify the signature on these nightly releases, you need `ssh-keygen`. Given the release file `passgen.deb`, the signature `passgen.deb.sig` and the `passgen.pub` file from this repository, run:

    $ ssh-keygen -Y verify -n file -s passgen.deb.sig -f passgen.pub -I nightly@passgen.it < passgen.deb
    Good "file" signature for nightly@passgen.it with ED25519 key SHA256:k7BsqKVzJMDEmgomupIE4VE9Xe4V4ffP506BLkz4JGQ

If the tool responds with anything other than "good file signature", do not trust the nightly build, preserve the files and contact the developers immediately.

## Usage

```
passgen version 1.0.0
Generate passwords from a regex-like pattern.
Usage: ./src/passgen/passgen [OPTIONS] [PATTERN]

PATTERN is a regex-like string describing the password.
    abc|def           Matches string 'abc' or 'def' (choice).
    [a-cf]            Matches character 'a', 'b', 'c', and 'f' (range).
    (abc)             Matches strings 'abc' (group).
    [a-c]{2,3}        Matches between 2 and 3 repetition of element (repeat).

OPTIONS
    -a, --amount      The amount of passwords
    -z, --null        Use NUL instead of newline to separate passes.
    -h, --help        Show this help information
    -p, --preset      Use the given preset.
    -v, --version     Show the version of this build.
    -d, --depth       Limit the parsing depth.
    -c, --complexity  Output complexity for each password.
    -r, --random STR  Which source of randomness to use. Can be:
                          file:/path/to/file, to use the file
                          xor:1234, to use xorshift with the given seed.
    -w, --wordlist NAME:PATH
                      Load a wordlist. For example:
                          german:/usr/share/dict/ngerman
                          english:/usr/share/dict/british-english

PRESETS
    apple1            Generate passwords like 'oKC-T37-Dew-Qyn'.
    apple2            Generate passwords like 'mHXr4X-CiK4w6-hbjF7T'.
```

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

[passgen-linux-amd64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-amd64.tar.xz
[passgen-linux-amd64.deb]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-amd64.deb
[passgen-linux-musl-amd64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-musl-amd64.tar.xz
[passgen-linux-aarch64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-aarch64.tar.xz
[passgen-linux-aarch64.deb]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-aarch64.deb
[passgen-linux-riscv64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-riscv64.tar.xz
[passgen-linux-riscv64.deb]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-riscv64.deb
[passgen-macos-amd64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-macos-amd64.tar.xz
[passgen-win64-amd64.zip]: https://xfbs.gitlab.io/passgen/nightly/passgen-win64-amd64.zip

[passgen-linux-amd64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-amd64.tar.xz.sig
[passgen-linux-amd64.deb.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-amd64.deb.sig
[passgen-linux-musl-amd64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-musl-amd64.tar.xz.sig
[passgen-linux-aarch64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-aarch64.tar.xz.sig
[passgen-linux-aarch64.deb.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-aarch64.deb.sig
[passgen-linux-riscv64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-riscv64.tar.xz.sig
[passgen-linux-riscv64.deb.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-riscv64.deb.sig
[passgen-macos-amd64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-macos-amd64.tar.xz.sig
[passgen-win64-amd64.zip.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-win64-amd64.zip.sig

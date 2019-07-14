# Passgen [![Build Status](https://travis-ci.org/xfbs/passgen.svg?branch=master)](https://travis-ci.org/xfbs/passgen) ![GitHub tag (latest SemVer)](https://img.shields.io/github/tag/xfbs/passgen.svg)

Passgen is a tool to generate passwords from a regex-like format string.

## Usage

```
Usage: passgen [OPTIONS] [FORMAT]
  -h, --help         Show this usage information
  -a, --amount       The amount of passwords
```

## Examples

Create 12-digit alphanumeric password.

    $ passgen "[a-zA-Z0-9]{12}"
    DnJ4bOIGmZbj

## Installing

You can install this project by downloading the latest release, extracting it,
and running:

    make release
    make install

See the [notes](NOTES.md) for more information about building the project.

## Prior art

* [pwgen](https://linux.die.net/man/1/pwgen).
* [apg](https://linux.die.net/man/1/apg).

## License

MIT, see [License](LICENSE) file.

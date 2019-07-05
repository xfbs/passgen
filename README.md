# Passgen

Passgen generates passwords. It takes a format string, which is a kind of simplified regex, and creates a random string matching that. 

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

## Installation

Install `clang` and `make` and build.

    $ make

Install to `/usr/local/bin`.

	$ sudo make install

## Makefile targets

* `make format` formats the code with clang-format.
* `make debug` creates a debug build in `./build/debug`.
* `make release` creates a release build in `./build/release`.

## License

MIT, see [License](LICENSE) file.

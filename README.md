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

## Building

Use the Makefile to build the project. For example, to build it in release mode, do:

    $ make release

When compiling, resulting files are placed in `./build/`. There are multiple targets:

* `release` for a release build with optimisations enabled.
* `debug` for a debug build without optimisations, with debug symbols and `DEBUG` defined.
* `debug-address` for a debug build with LLVM AddressSanitizer.
* `debug-memory` for a debug build with LLVM MemorySanitizer.
* `debug-undefined` for a debug build with LLVM UndefinedSanitizer.

## Actions

* `make format` to use clang-format to clean up the code.
* `make clean` to remove all build output.

## License

MIT, see [License](LICENSE) file.

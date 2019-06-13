# Passgen

Generate password.

```
Usage: passgen [-h|-a|-l|-c|-s|-as|-ac] ...
  -h|--help         Show this usage information
  -a|--amount       The amount of passwords
  -l|--length       The length of the passwords
  -c|--custom       Include custom characters
  -s|--symbols      Include symbols in the passwords
  -n|--numeric      Include numeric characters
  --alpha           Include alphabetical characters
  -as|--alpha-small Include small alphabetical characters
  -ac|--alpha-caps  Include capitalized alphabetical characters
```

## Installation

Install `clang` and `make` and build.

    $ make

Install to `/usr/local/bin`.

	$ sudo make install

## License

MIT.

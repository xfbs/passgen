# Notes

## Version

When bumping the version, be sure to change it in all places:

* Makefile
* man/passgen.1
* Code

## Todo

### Unicode Support

To support generating passwords in truly any language, the pattern
parser needs to be adjusted to understand unicode. For this, libraries
for parsing and emitting UTF-8 are necessary.

### Using getrandom

Reading from `/dev/urandom` could be replaced with the `getrandom()`
syscall.

### Word List

Supporting the `\w` special character, along with a wordlist, should enable
[XKCD-936](https://www.xkcd.com/936/)-compliant passwords.

### Pronounceable character strings

Supporting some kind of mechanism to generated pronounceable passwords should
be implemented.

### Supporting escape codes

Supporting escape codes like `\n`, `\t`, `\uAAFF`, `\xAF`, etc. should be
implemented.

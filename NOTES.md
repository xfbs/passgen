# Notes

## Version

When bumping the version, be sure to change it in all places:

* Makefile
* man/passgen.1
* Code

## Generating documentation

The documentation is published in the `gh-pages` branch. Use this workflow to
update it:

    $ git worktree add docs/html gh-pages
    $ make docs
    $ cd docs/html
    $ git add .
    $ git commit -m "updates docs"
    $ git push origin gh-pages
    $ cd ../../

To undo the worktree, do (from the main project dir)

    $ git worktree remove docs/html
    $ make clean

## Generating debian package

Experimental. There's a tutorial for it [here](http://www.tldp.org/HOWTO/html_single/Debian-Binary-Package-Building-HOWTO/). Also see [this](https://www.debian.org/doc/debian-policy/ch-source.html#debian-changelog-debian-changelog).

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

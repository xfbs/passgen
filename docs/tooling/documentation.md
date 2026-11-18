# Documentation

The documentation is in three places:

- The documentation for users is on the website, [passgen.io][], which has
  its own [repository][website]. The man page (`man/passgen.1.in`) is the
  reference for the command-line tool, and is in this repository.
- The documentation for developers is what you are reading. It is plain
  Markdown in the `docs/` folder, meant to be read on GitLab or GitHub.
- The documentation of the API of `libpassgen` is in the headers, as comments,
  and is turned into HTML with [Doxygen][]. Its main page is `docs/api.md`,
  which includes the examples in `examples/`.

## Man page

The man page is written in the `mdoc` language. To check it for mistakes, and
to see what it looks like, run `mandoc` on the one which the build generates
(which has the version filled in):

    mandoc -Tlint build/passgen.1
    mandoc build/passgen.1 | less

## API documentation

Both `doxygen` and `graphviz` need to be installed. To generate the
documentation, run Doxygen in the root of the repository:

    doxygen Doxyfile

The result is placed in `docs/html`. There should not be any warnings about
the headers or the pages.

The examples in `examples/` are built against the shared library, with only
the public headers, and run as tests, so that what the documentation shows
keeps working. Comments in them use `//`, as Doxygen leaves out `///`
comments in code which it includes.

Continuous integration generates the documentation for every commit to the
`master` branch, and publishes it at
[passgen.gitlab.io/passgen/docs][api].

It also measures how much of the API is documented, with
`scripts/docs_coverage.py`.

[passgen.io]: https://passgen.io
[website]: https://gitlab.com/passgen/website
[Doxygen]: https://www.doxygen.nl/
[api]: https://passgen.gitlab.io/passgen/docs/

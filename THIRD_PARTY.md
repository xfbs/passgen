# Third-party code

Passgen itself is licensed under the MIT license, see [LICENSE.md](LICENSE.md).
It includes some code which was written by others, under the following
licenses. The files have the full text of their license at the top.

## Part of the library and the command-line tool

| Code | Files | License |
| --- | --- | --- |
| [Monocypher](https://monocypher.org/) 4.0.2, by Loup Vaillant and others. Used for Argon2 and ChaCha20, with its symbols renamed. | `src/monocypher.c`, `src/include/passgen/monocypher.h` | BSD-2-Clause or CC0-1.0 |
| [SipHash](https://github.com/veorq/SipHash) reference implementation, by Jean-Philippe Aumasson and Daniel J. Bernstein. | `src/util/siphash.c` | CC0-1.0 |
| [UTF-8 decoder](https://bjoern.hoehrmann.de/utf-8/decoder/dfa/), by Björn Höhrmann. | `src/util/utf8.c` | MIT |
| UTF-8 encoder, by Ondřej Hruška. | `src/util/utf8.c` | MIT |

## Only used when building

These are not part of what is built.

| Code | Files | License |
| --- | --- | --- |
| CMake modules by George Cave. | `cmake/CodeCoverage.cmake`, `cmake/Sanitizers.cmake` | Apache-2.0 |
| CMake modules by Tomas Zeman. | `cmake/ClangFormat.cmake`, `cmake/IWYU.cmake` | BSL-1.0 |

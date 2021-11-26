# Development

Development on passgen happens at [GitLab][gitlab]. [Merge requests][merge-requests] and [bug reports][bug-reports] are more than welcome there. You may also use the [GitHub][github] project page, which is kept somewhat in sync.

[GitLab CI][ci] is used to test and build the code on every commit, for all of the platforms that are supported. [LLVM sanitizers][llvm-sanitizer] and [valgrind][valgrind] are used to catch memory unsafety errors. [QEMU][qemu] is used to execute all of the unit tests for all of the supported platforms. Additionally, the CI generates SSH signatures for the nightly builds that are also available for download at the [releases](releases.md) page.

[Doxygen documentation][doxygen] is published for the source code on every commit to the `master` branch. Finally, code coverage (as determined by the unit tests) is generated by [lcov][coverage-lcov] and [llvm-cov][coverage-llvm-cov], and published similarly (see the respective links).

Passgen is written in C, consisting of a library and a command-line utility that uses the library. It can be easily embedded into other applications. Passgen does not use any global state, and as such it is safe to use from different threads. However, the parser can only be used from one thread at a time (it has no mutex to prevent errors from happening due to concurrent access).

Passgen uses the [CMake][cmake] build system, allowing it to be built on different platforms and with different compilers. However, only `gcc` and `clang` are officially supported and tested, as those are used in the CI builds.

[gitlab]: https://gitlab.com/xfbs/passgen
[github]: https://github.com/xfbs/passgen
[doxygen]: https://xfbs.gitlab.io/passgen/doxygen/html
[merge-requests]: https://gitlab.com/xfbs/passgen/-/merge_requests
[bug-reports]: https://gitlab.com/xfbs/passgen/-/issues
[coverage-lcov]: https://xfbs.gitlab.io/passgen/coverage/lcov/
[coverage-llvm-cov]: https://xfbs.gitlab.io/passgen/coverage/llvm-cov/
[cmake]: https://cmake.org/
[llvm-sanitizer]: https://clang.llvm.org/docs/AddressSanitizer.html
[valgrind]: https://valgrind.org/
[qemu]: https://www.qemu.org/
[ci]: https://gitlab.com/xfbs/passgen/-/pipelines
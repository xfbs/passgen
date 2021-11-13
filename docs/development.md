# Development

Development on passgen happens at [GitLab](https://gitlab.com/xfbs/passgen). We are more than happy to accept merge requests there, as well as bug reports on the issue tracker. You may also use the [GitHub](https://github.com/xfbs/passgen) project page, which is kept in sync.

We use the amazing GitLab CI to test and build the code on every commit, for all of the platforms that we support. We (try to) have extensive unit tests, and we run them for every architecture that we support using the amazing QEMU project. We use the CI as well to generate signed nightly builds that are available on the [releases](releases.md) page.

We use code coverage tools like gcov and llvm-cov to generate unit test coverage reports on every commit or merge to master.


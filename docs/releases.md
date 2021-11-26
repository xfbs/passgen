# Releases

As of right now, there are no real releases for passgen, but we do have nightly builds from the master branch that you can download and try out. Currently, we support a variety of platforms for our Linux builds and have working unit tests for all platforms. We also offer macOS and MS Windows builds, however these are considered experimental because there is no easy way to run unit tests for them right now.

## Linux

| Architecture | Tarball | Debian |
| --- | --- | --- |
| AMD64 | [passgen-linux-amd64.tar.xz][] ([sig][passgen-linux-amd64.tar.xz.sig]) | [passgen-linux-amd64.deb][] ([sig][passgen-linux-amd64.deb.sig]) |
| ARM64 | [passgen-linux-aarch64.tar.xz][] ([sig][passgen-linux-aarch64.tar.xz.sig]) | [passgen-linux-aarch64.deb][] ([sig][passgen-linux-aarch64.deb.sig]) |
| RISC-V 64biy | [passgen-linux-riscv64.tar.xz][] ([sig][passgen-linux-riscv64.tar.xz.sig]) | [passgen-linux-riscv64.deb][] ([sig][passgen-linux-riscv64.deb.sig]) |

It should be fairly easy to add more architectures, please reach out if you want to help with that.

## macOS

*Warning: the macOS releases are considered experimental because they are untested.*  
M1 Mac builds are work-in-progress.

| Architecture | Tarball |
| --- | --- |
| AMD64 | [passgen-macos-amd64.tar.xz][] ([sig][passgen-macos-amd64.tar.xz.sig]) |

## MS Windows

*Warning: the win64 releases are considered experimental because they are untested.*

| Architecture | ZIP Archive |
| --- | --- |
| AMD64 | [passgen-win64-amd64.zip][] ([sig][passgen-win64-amd64.zip.sig]) |

# Verifying

Every release comes with a signature file. This can be used to verify the authenticity and integrity of the releases. To do so, you need `ssh-keygen` and the passgen developer public key.

```bash
wget https://xfbs.gitlab.com/passgen/passgen.pub
ssh-keygen -Y verify -f passgen.pub -s passgen-linux-amd64.deb.sig -n name -I nightly@passgen.it < passgen-linux-amd64.deb
Good "file" signature for nightly@passgen.it with ED25519 key SHA256:k7BsqKVzJMDEmgomupIE4VE9Xe4V4ffP506BLkz4JGQ
```

If the verification fails, please do not trust the build and contact the developers immediately!

[passgen-linux-amd64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-amd64.tar.xz
[passgen-linux-amd64.deb]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-amd64.deb
[passgen-linux-musl-amd64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-musl-amd64.tar.xz
[passgen-linux-aarch64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-aarch64.tar.xz
[passgen-linux-aarch64.deb]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-aarch64.deb
[passgen-linux-riscv64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-riscv64.tar.xz
[passgen-linux-riscv64.deb]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-riscv64.deb
[passgen-macos-amd64.tar.xz]: https://xfbs.gitlab.io/passgen/nightly/passgen-macos-amd64.tar.xz
[passgen-win64-amd64.zip]: https://xfbs.gitlab.io/passgen/nightly/passgen-win64-amd64.zip

[passgen-linux-amd64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-amd64.tar.xz.sig
[passgen-linux-amd64.deb.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-amd64.deb.sig
[passgen-linux-musl-amd64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-musl-amd64.tar.xz.sig
[passgen-linux-aarch64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-aarch64.tar.xz.sig
[passgen-linux-aarch64.deb.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-aarch64.deb.sig
[passgen-linux-riscv64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-riscv64.tar.xz.sig
[passgen-linux-riscv64.deb.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-linux-riscv64.deb.sig
[passgen-macos-amd64.tar.xz.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-macos-amd64.tar.xz.sig
[passgen-win64-amd64.zip.sig]: https://xfbs.gitlab.io/passgen/nightly/passgen-win64-amd64.zip.sig

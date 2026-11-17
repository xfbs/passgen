# Nightly

As of right now, there are no real releases for passgen, but there are nightly builds from the `master` branch that you can download and try out. Currently, passgen supports a variety of platforms for the Linux builds, as well as macOS and Windows builds. The latter however are considered experimental at this point because they are not tested as part of the CI process.

## Downloads

!!! warning "Windows and macOS builds are currently broken"

    The Windows and macOS builds are currently broken. We are working on fixing them. Please
    try using the Linux builds inside WSL2 or a VM or give the WebAssembly builds a try in the
    meantime.

| Architecture | Links |
| --- | --- |
| Linux amd64 | [passgen-nightly-linux-amd64.tar.xz][] ([sig][passgen-nightly-linux-amd64.tar.xz.sig])<br />[passgen-nightly-linux-amd64.deb][] ([sig][passgen-nightly-linux-amd64.deb.sig])<br />[passgen-nightly-linux-amd64-musl.tar.xz][] ([sig][passgen-nightly-linux-amd64-musl.tar.xz.sig])<br />[passgen-nightly-linux-amd64-musl.bin][] ([sig][passgen-nightly-linux-amd64-musl.bin.sig]) |
| Linux arm64 | [passgen-nightly-linux-aarch64.tar.xz][] ([sig][passgen-nightly-linux-aarch64.tar.xz.sig])<br />[passgen-nightly-linux-arm64.deb][] ([sig][passgen-nightly-linux-arm64.deb.sig]) |
| Linux riscv64 | [passgen-nightly-linux-riscv64.tar.xz][] ([sig][passgen-nightly-linux-riscv64.tar.xz.sig])<br />[passgen-nightly-linux-riscv64.deb][] ([sig][passgen-nightly-linux-riscv64.deb.sig]) |
| macOS amd64 *experimental* | [passgen-nightly-macos-amd64.tar.xz][] ([sig][passgen-nightly-macos-amd64.tar.xz.sig]) |
| Windows amd64 | [passgen-nightly-win64-amd64.zip][] ([sig][passgen-nightly-win64-amd64.zip.sig]) |
| [WebAssembly WASI](wasi.md) | [passgen-nightly-wasi.wasm][] ([sig][passgen-nightly-wasi.wasm.sig]) |

# Verifying

Every release comes with a signature file. This can be used to verify the authenticity and integrity of the releases. To do so, you need `ssh-keygen` and the passgen developer public key.

```bash
wget https://passgen.gitlab.io/passgen/passgen.pub
ssh-keygen -Y verify -f passgen.pub -s passgen-nightly-linux-amd64.deb.sig -n file -I nightly@passgen.it < passgen-nightly-linux-amd64.deb
Good "file" signature for nightly@passgen.it with ED25519 key SHA256:k7BsqKVzJMDEmgomupIE4VE9Xe4V4ffP506BLkz4JGQ
```

If the verification fails, please do not trust the build and contact the developers immediately!

[passgen-nightly-linux-amd64.tar.xz]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-amd64.tar.xz
[passgen-nightly-linux-amd64.deb]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-amd64.deb
[passgen-nightly-linux-amd64-musl.tar.xz]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-amd64-musl.tar.xz
[passgen-nightly-linux-amd64-musl.bin]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-amd64-musl.bin
[passgen-nightly-linux-aarch64.tar.xz]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-aarch64.tar.xz
[passgen-nightly-linux-arm64.deb]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-arm64.deb
[passgen-nightly-linux-riscv64.tar.xz]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-riscv64.tar.xz
[passgen-nightly-linux-riscv64.deb]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-riscv64.deb
[passgen-nightly-macos-amd64.tar.xz]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-macos-amd64.tar.xz
[passgen-nightly-win64-amd64.zip]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-win64-amd64.zip
[passgen-nightly-wasi.wasm]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-wasi.wasm

[passgen-nightly-linux-amd64.tar.xz.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-amd64.tar.xz.sig
[passgen-nightly-linux-amd64.deb.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-amd64.deb.sig
[passgen-nightly-linux-amd64-musl.tar.xz.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-amd64-musl.tar.xz.sig
[passgen-nightly-linux-amd64-musl.bin.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-amd64-musl.bin.sig
[passgen-nightly-linux-aarch64.tar.xz.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-aarch64.tar.xz.sig
[passgen-nightly-linux-arm64.deb.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-arm64.deb.sig
[passgen-nightly-linux-riscv64.tar.xz.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-riscv64.tar.xz.sig
[passgen-nightly-linux-riscv64.deb.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-linux-riscv64.deb.sig
[passgen-nightly-macos-amd64.tar.xz.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-macos-amd64.tar.xz.sig
[passgen-nightly-win64-amd64.zip.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-win64-amd64.zip.sig
[passgen-nightly-wasi.wasm.sig]: https://passgen.gitlab.io/passgen/nightly/passgen-nightly-wasi.wasm.sig

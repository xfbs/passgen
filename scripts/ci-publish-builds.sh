#!/bin/bash
# Collects the builds from the build jobs, and signs them, to be published as
# the nightly builds. The names all start with `passgen-nightly-`, so that they
# cannot be mistaken for releases. They don't have a version in them, so that
# they can be linked to.
set -euxo pipefail

out=public/nightly
mkdir -p "$out"

cp build-alpine-release/passgen-*-Linux.tar.xz "$out/passgen-nightly-linux-amd64-musl.tar.xz"
cp build-alpine-release/src/cli/passgen "$out/passgen-nightly-linux-amd64-musl.bin"

cp build-amd64/passgen-*-Linux.tar.xz "$out/passgen-nightly-linux-amd64.tar.xz"
cp build-amd64/passgen_*_amd64.deb "$out/passgen-nightly-linux-amd64.deb"

cp build-aarch64/passgen-*-Linux.tar.xz "$out/passgen-nightly-linux-aarch64.tar.xz"
cp build-aarch64/passgen_*_arm64.deb "$out/passgen-nightly-linux-arm64.deb"

cp build-riscv64/passgen-*-Linux.tar.xz "$out/passgen-nightly-linux-riscv64.tar.xz"
cp build-riscv64/passgen_*_riscv64.deb "$out/passgen-nightly-linux-riscv64.deb"

cp build-wasi/src/cli/passgen "$out/passgen-nightly-wasi.wasm"

# sign every one of them, which adds a `.sig` file next to it.
chmod 0600 "$PASSGEN_SIGNING_KEY"
for file in "$out"/passgen-nightly-*; do
    ssh-keygen -Y sign -f "$PASSGEN_SIGNING_KEY" -n file "$file"
done

# export keys file
cp data/passgen.pub public/passgen.pub

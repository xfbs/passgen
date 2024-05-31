#!/bin/bash
set -euxo pipefail

mkdir -p public/nightly

cp build-alpine-release/passgen-*-Linux.tar.xz public/passgen-linux-amd64-musl.tar.xz
cp build-alpine-release/src/cli/passgen public/passgen-linux-amd64-musl.bin

cp build-amd64/passgen-*-Linux.tar.xz public/passgen-linux-amd64.tar.xz
cp build-amd64/passgen_*_amd64.deb public/passgen-linux-amd64.deb

cp build-aarch64/passgen-*-Linux.tar.xz public/passgen-linux-aarch64.tar.xz
cp build-aarch64/passgen_*_arm64.deb public/passgen-linux-arm64.deb

cp build-riscv64/passgen-*-Linux.tar.xz public/passgen-linux-riscv64.tar.xz
cp build-riscv64/passgen_*_riscv64.deb public/passgen-linux-riscv64.deb

cp build-macos/passgen-*-Darwin.tar.xz public/passgen-macos-amd64.tar.xz

cp build-mingw64/passgen-*-win64.zip public/passgen-win64-amd64.zip

cp build-wasi/src/cli/passgen public/passgen-wasi.wasm

chmod 0600 $PASSGEN_SIGNING_KEY

function sign() {
    ssh-keygen -Y sign -f $PASSGEN_SIGNING_KEY -n file $1
}

sign public/passgen-linux-amd64-musl.tar.xz
sign public/passgen-linux-amd64-musl.bin
sign public/passgen-linux-amd64.tar.xz
sign public/passgen-linux-amd64.deb
sign public/passgen-linux-aarch64.tar.xz
sign public/passgen-linux-arm64.deb
sign public/passgen-linux-riscv64.tar.xz
sign public/passgen-linux-riscv64.deb
sign public/passgen-macos-amd64.tar.xz
sign public/passgen-win64-amd64.zip
sign public/passgen-wasi.wasm

# export keys file
cp data/passgen.pub public/passgen.pub

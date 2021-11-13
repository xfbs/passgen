#!/bin/bash
set -euxo pipefail

mkdir -p public/nightly

cp build-alpine-release/passgen-*-Linux.tar.xz public/nightly/passgen-linux-musl-amd64.tar.xz
cp build-amd64/passgen-*-Linux.tar.xz public/nightly/passgen-linux-amd64.tar.xz
cp build-amd64/passgen_*_amd64.deb public/nightly/passgen-linux-amd64.deb
cp build-riscv64/passgen-*-Linux.tar.xz public/nightly/passgen-linux-riscv64.tar.xz
cp build-riscv64/passgen_*_riscv64.deb public/nightly/passgen-linux-riscv64.deb
cp build-macos/passgen-*-Darwin.tar.xz public/nightly/passgen-macos-amd64.tar.xz
cp build-mingw64/passgen-*-win64.zip public/nightly/passgen-win64-amd64.zip

chmod 0600 $PASSGEN_SIGNING_KEY

function sign() {
    ssh-keygen -Y sign -f $PASSGEN_SIGNING_KEY -n file $1
}

sign public/nightly/passgen-linux-musl-amd64.tar.xz
sign public/nightly/passgen-linux-amd64.tar.xz
sign public/nightly/passgen-linux-amd64.deb
sign public/nightly/passgen-linux-riscv64.tar.xz
sign public/nightly/passgen-linux-riscv64.deb
sign public/nightly/passgen-macos-amd64.tar.xz
sign public/nightly/passgen-win64-amd64.zip

# export keys file
cp passgen.pub public/passgen.pub

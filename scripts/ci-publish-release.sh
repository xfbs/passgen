#!/bin/bash
set -euxo pipefail

chmod 0600 $PASSGEN_SIGNING_KEY

function sign() {
    ssh-keygen -Y sign -f $PASSGEN_SIGNING_KEY -n file $1
}

function upload() {
    curl --header "JOB-TOKEN: ${CI_JOB_TOKEN}" --upload-file "$1" "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/passgen-${CI_COMMIT_TAG}/$2"
}

function release() {
    sign "$1"
    upload "$1" "$2"
    upload "$1.sig" "$2.sig"
}

# release binaries
release build-alpine-release/passgen-*-Linux.tar.xz public/passgen-linux-amd64-musl.tar.xz
release build-alpine-release/src/cli/passgen public/passgen-linux-amd64-musl.bin
release build-amd64/passgen-*-Linux.tar.xz public/passgen-linux-amd64.tar.xz
release build-amd64/passgen_*_amd64.deb public/passgen-linux-amd64.deb
release build-aarch64/passgen-*-Linux.tar.xz public/passgen-linux-aarch64.tar.xz
release build-aarch64/passgen_*_arm64.deb public/passgen-linux-arm64.deb
release build-riscv64/passgen-*-Linux.tar.xz public/passgen-linux-riscv64.tar.xz
release build-riscv64/passgen_*_riscv64.deb public/passgen-linux-riscv64.deb
release build-macos/passgen-*-Darwin.tar.xz public/passgen-macos-amd64.tar.xz
release build-mingw64/passgen-*-win64.zip public/passgen-win64-amd64.zip
release build-wasi/src/cli/passgen public/passgen-wasi.wasm

upload data/passgen.pub passgen.pub

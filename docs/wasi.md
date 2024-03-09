# WASI Build

Passgen can be built as a [WebAssembly](https://webassembly.org/) executable following the [WASI](https://wasi.dev) standard. This means it can run portably on any operating system, and it is sandboxed and has no access to anything that is not explicitly granted. These properties should, in theory, make it more secure.

If you want to try it, you can download the latest WASI build from the [releases](nightly.md) page. To run passgen, you will need to install a WASI-compatible runtime, and you need to specify some flags to get it to work.

## WASI Runtime

One runtime I can recommend is the excellent [WasmTime](https://wasmtime.dev/) runtime, which is written in Rust and uses the Cranelift optimizer. Instructions on how to install it are [here](https://docs.wasmtime.dev/cli-install.html). If you have cargo, you can install it simply by executing:

    cargo install wasmtime-cli

Otherwise, they provide a convenient bash script to get it up and running:

    curl https://wasmtime.dev/install.sh -sSf | bash

Once installed, you should be able to run it:

    wasmtime -V
    wasmtime-cli 4.0.0

## Running Passgen

Running passgen with the WasmTime runtime is easy, but it needs some permissions in order to be able to access random data and your home directory (to load your configuration). Here is how:

    wasmtime run --dir /dev --dir ~ --env HOME=$HOME passgen-wasi.wasm -- -p apple1
    QZv-Wk9-UOU-IN6

    wasmtime run --dir /dev --dir ~ --env HOME=$HOME passgen-wasi.wasm -- -p uuid
    942e6cbf-50b6-4bc6-8675-947f8fe85031

## Issues

If you run into any issues with the WASI build of Passgen, please reach out by creating an issue in the [repository](https://gitlab.com/xfbs/passgen).

FROM alpine
RUN apk add clang binaryen wasi-sdk cmake make lld git llvm ruby
ENV CC="clang -target wasm32-wasi --sysroot /usr/share/wasi-sysroot/"

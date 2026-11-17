FROM alpine
RUN apk add clang binaryen wasi-sdk cmake make lld git llvm ruby
# the target used to be called wasm32-wasi, which clang has deprecated and
# which newer versions of wasi-libc no longer ship libraries for.
ENV CC="clang --target=wasm32-wasip1 --sysroot /usr/share/wasi-sysroot/"

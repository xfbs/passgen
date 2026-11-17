# Sanitizers

LLVM sanitizers instrument the code at compile-time to catch various illegal
behaviours at runtime. Enabling them needs support from the build system,
unlike Valgrind which works with any compiled binary. The advantage of the
sanitizers is that they can catch more kinds of bugs.

These are the sanitizers that currently exists:

| Name | Description |
| --- | --- |
| [AddressSanitizer] | Memory error detector. Can detect out-of-bounds access to heap, stack and globals, use-after-free, use-after-return, use-after-scope, double-free, invalid-free and memory leaks. |
| [ThreadSanitizer] | Data race detector. |
| [MemorySanitizer] | Uninitialized read detector. |
| [UndefinedBehaviourSanitizer] | Undefined behavior detector. Can detect array subscript out-of-bounds, bitwise shifts out-of-bounds, dereferencing misaligned or NULL pointers, signed integer overflow, conversion to or from floating-point which results in overflow. |

[AddressSanitizer]: https://clang.llvm.org/docs/AddressSanitizer.html
[ThreadSanitizer]: https://clang.llvm.org/docs/ThreadSanitizer.html
[MemorySanitizer]: https://clang.llvm.org/docs/MemorySanitizer.html
[UndefinedBehaviourSanitizer]: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html

## Quick Run

In the repository root, there is a Makefile with some handy targets. These will
create a new build folder with the sanitizer enabled, and then run unit tests.

    make asan
    make msan
    make usan

There is also a script for compiling the code with a sanitizer in a temporary
folder and running tests, this can be done by running the following, using the
Address sanitizer for example:

    ./scripts/sanitizer.sh Address

Supported Sanitizers and Sanitizer combinations are *Address*, *Memory*,
*MemoryWithOrigins*, *Undefined*, *Thread*, *Leak*, *Address;Undefined*.

## Enabling Sanitizers

Enabling a specific sanitizer can be done by setting the `USE_SANITIZER` CMake
variable when initializing the build folder. 

    cmake -DUSE_SANITIZER=Address

Supported Sanitizers and Sanitizer combinations are *Address*, *Memory*,
*MemoryWithOrigins*, *Undefined*, *Thread*, *Leak*, *Address;Undefined*.

It is useful to turn on these sanitizers when running unit tests, because they
catch more bugs that way. It is not recommended to leave the sanitizers enabled
for release builds of Passgen, because they do add overhead (depending on the
sanitizer).

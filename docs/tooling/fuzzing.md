# Fuzzing

Fuzzing means running code with a large number of generated inputs, to find
ones which make it misbehave. Passgen parses patterns, wordlists and
descriptions of randomness sources, all of which may come from someone else, so
this is a good way to find bugs in it.

The fuzzing harnesses are in `src/fuzz`. Each of them is a function which takes
some bytes, and feeds them to a part of the library:

| Harness | What the input is |
| --- | --- |
| `fuzz-pattern` | A pattern, which is parsed and then generated from. |
| `fuzz-wordlist` | The length of the Markov chain (first byte) and a wordlist, which is used to generate words. |
| `fuzz-random` | The description of a randomness source, as passed to `--random`. |
| `fuzz-utf8` | Text to decode as UTF-8, and encode again. |

## Corpus

For every harness, there is a folder of inputs in `src/fuzz/corpus`. These are
used as a starting point when fuzzing, and they are also run as part of the
tests (the tests named `fuzz-*`), with whatever sanitizers the build has
enabled. This does not need a fuzzing engine, so it works with any compiler.

When fuzzing finds an input that causes a problem, add it (or a reduced version
of it) to the corpus when fixing the problem. That way, it is checked on every
build from then on.

## Running

To look for new inputs, the harnesses need to be built with [libFuzzer][],
which is part of Clang, and with sanitizers, as those are what notices most
problems. There is a script which does all of that, and then runs every
harness for a minute:

    ./scripts/fuzz.sh
    # or
    make fuzz

It takes the number of seconds to run each harness for, and which harnesses
to run:

    ./scripts/fuzz.sh 600 pattern wordlist

Everything ends up in `build-fuzz`. When a harness finds an input which
causes a problem, it stops, and the input is saved in `build-fuzz/crashes`.
It can be passed to the harness to reproduce the problem:

    ./build-fuzz/src/fuzz/fuzz-pattern build-fuzz/crashes/pattern/crash-...

The inputs found along the way are kept in `build-fuzz/work`, so running the
script again continues where it left off.

It is worth fuzzing a release build as well, as assertions are only active in
debug builds, so the two find different problems:

    BUILD_TYPE=Release ./scripts/fuzz.sh

Note that the Clang which comes with Xcode on macOS does not include
libFuzzer. The script uses the one from Homebrew (`brew install llvm`) if it
is there. Set `CC` to use another one.

### By hand

The script enables the `PASSGEN_FUZZ` option of the build system, which builds
the harnesses with libFuzzer rather than for running over their corpus:

    CC=clang cmake -S . -B build-fuzz \
        -DPASSGEN_FUZZ=ON \
        -DCMAKE_BUILD_TYPE=Debug \
        "-DUSE_SANITIZER=Address;Undefined"
    cmake --build build-fuzz

A harness takes a folder to keep the inputs it finds in, and the corpus to
start from, along with the [options of libFuzzer][libFuzzer]:

    mkdir -p work
    ./build-fuzz/src/fuzz/fuzz-pattern work src/fuzz/corpus/pattern \
        -dict=src/fuzz/pattern.dict -max_len=512 -max_total_time=300

## Limits

Generating from a pattern can take arbitrarily long, as a short pattern such as
`(a{999}){999}` results in a lot of output. To keep every run short, the
harnesses stop generating after some amount of output or randomness, and skip
patterns with long numbers in them. They also do not open files, so the `file:`
randomness source and the configuration file are not covered.

[libFuzzer]: https://llvm.org/docs/LibFuzzer.html

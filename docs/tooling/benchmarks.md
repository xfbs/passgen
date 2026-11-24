# Benchmarks

The benchmarks measure how fast the parts of Passgen are, from the sources of
randomness and the data structures up to what a user waits for: reading a
wordlist, building a Markov chain from it, and generating. They are built as
`passgen-bench` (with the `BUILD_BENCH` option, which is on by default), from
the sources in `src/bench`.

Measure with a release build, as a debug build is several times slower, and
not in the same places:

    cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release --target passgen-bench
    ./build-release/src/bench/passgen-bench

## Wordlist

The benchmarks for wordlists, Markov chains and generating from them use the
large EFF wordlist, which is downloaded into `data/` by:

    ./scripts/fetch-wordlists.sh

Without it, they are skipped. Another wordlist can be given with `-o
wordlist=PATH`, such as a large one, where building Markov chains takes
longest:

    passgen-bench -o wordlist=/usr/share/dict/words markov

## Running

Without arguments, every benchmark is run, for a second each. With arguments,
the benchmarks whose names contain one of them are run. `-l` lists them.

    passgen-bench generate markov_build_3
    passgen-bench -t 5s wordlist_load

| Option | What it does |
| --- | --- |
| `-l`, `--list` | Lists the benchmarks. |
| `-t`, `--time TIME` | How long to run each benchmark, such as `2`, `500ms` or `1m`. Defaults to a second. |
| `-i`, `--iterations N` | The fewest samples to take of each, even if that takes longer. Defaults to 10. |
| `-o`, `--option NAME=VALUE` | Sets an option of the benchmarks: `wordlist`, and `level` for `master_derive`. |
| `-s`, `--oneshot NAME` | Runs one benchmark once (or `-i` times) without measuring, for profilers. |
| `--tsv` | Prints the results as tab-separated values, for comparing them. |

`master_derive`, which derives the key from a master passphrase, is meant to
be slow and use a lot of memory. It only runs when it is named.

## Results

    benchmark                           throughput    iteration   spread  samples
    generate_apple2                2.70 Moutputs/s     37.05 us     1.2%      418

A benchmark is run in batches of iterations (an iteration of `generate_apple2`
generates 100 passwords), which are long enough to be timed precisely, at least
a millisecond. Each batch is a sample of how long an iteration takes. The
first batches find how many iterations that needs, and warm up the caches,
and are not counted.

- **throughput**: how much an iteration processes (bytes, words, outputs),
  divided by the time it takes.
- **iteration**: the median time of an iteration.
- **spread**: half of the range which the middle half of the samples are in,
  relative to the median. A large one means that something else was running,
  or that the benchmark is not doing the same in every iteration.
- **samples**: how many batches were timed.

What a benchmark needs (a wordlist read for building a Markov chain, a fresh
environment for resolving presets) is prepared before it is timed, and what it
returns is freed after.

## Comparing

To see whether a change makes something faster or slower, save a run from
before and after it, and compare them:

    passgen-bench --tsv > before.tsv
    # make the change, and build again
    passgen-bench --tsv > after.tsv
    ./scripts/bench-compare.py before.tsv after.tsv

A change is only called faster or slower when it is more than 5% (or what is
given with `--threshold`), and the middle halves of the samples of both runs
do not overlap. Otherwise it is noise.

Two runs of the same code still differ, by a few percent, and by more when
something else is running, or on a virtual machine or a processor with cores
of different speeds, which the system moves the benchmark between. So run on
an otherwise idle machine, and confirm a change with another pair of runs
before believing it.

## Profiling

`-s` runs a benchmark once without measuring anything, which is what to run
under a profiler, so that it only sees that benchmark:

    perf record ./build-release/src/bench/passgen-bench -s markov_build_3 -i 20
    perf report

    valgrind --tool=cachegrind ./build-release/src/bench/passgen-bench -s generate_markov

Cachegrind counts instructions rather than measuring time, so it gives the
same result on every run, which is useful for small changes.

## What was tried

### Building Markov chains

Building a Markov chain from the large EFF wordlist takes 2 to 3 ms at any
length, so it is only slow for large wordlists. With `/usr/share/dict/words`
(480,000 words), `markov_build_3` takes about 160 ms and `markov_build_8`
about 300 ms. This was measured in November 2026 on a virtual machine on
Apple silicon, so only the ratios mean much.

The time goes to following pointers through the nodes of the chain: every
character of every word walks from the root down through as many nodes as
the length of the chain, and most of those are not in the cache. It does not
go to growing the arrays (by one entry at a time) or searching them:

- The same words in a random order take much longer, 1.8 s rather than
  0.6 s at length 8, because in sorted order consecutive words share most of
  their path.
- At length 3, there are 4.95 million insertions, but only 135,669 entries
  in 23,556 leaves. Almost every insertion only counts one up.

Other ways to build the chain were tried in a prototype, in ms, with the
words sorted and shuffled:

| | Length 3 | shuffled | Length 8 | shuffled |
| --- | --- | --- | --- | --- |
| As it is | 182 | 301 | 624 | 1,762 |
| Sort every sequence, then insert | 523 | | 1,753 | |
| Count sequences in a hash table, sort, insert | 71 | 69 | 968 | 996 |
| Count sequences in a hash table, insert | 66 | 63 | 1,235 | 1,249 |

Counting in a hash table first makes length 3 about 2.5 times as fast for a
sorted wordlist and 4 times for a shuffled one, as the order of the words no
longer matters. At length 8 it is slower for a
sorted wordlist, because there are 1.6 million different sequences to sort
(qsort alone takes 600 ms). It would need about 150 lines and more memory
while building: about 10 MB at length 3, where a whole run of `passgen`
needs 14.5 MB at most, and 85 to 160 MB at length 8, where it needs 120 MB.

So the chains are built as they were: it saves a tenth of a second or two
on unusual wordlists, and nothing on the ones that are installed.

## Tests

Every benchmark is run once as part of the tests (`passgen-bench` and
`passgen-bench-master` in `ctest`), with the small wordlist in `examples/`.
That doesn't measure anything, it makes sure that they keep working, also with
the sanitizers. The test fails if a benchmark is skipped.

## Adding a benchmark

A benchmark is a `bench` (see `src/bench/bench.h`), which is listed in
`benches` in `src/bench/bench.c`. It has:

- `prepare`, which makes its input from the options. If it returns `NULL`, the
  benchmark is skipped, such as when a file it needs is not there.
- `iterate`, which is what is timed.
- `consumes`, for a benchmark whose iterations use up their input, which then
  gets a new one from `prepare` for every iteration. What `iterate` returns is
  given to `cleanup`.
- `release`, which frees the input otherwise.
- `multiplier` and `unit`, for how much an iteration processes.
- `optional`, for one which is only run when it is named.

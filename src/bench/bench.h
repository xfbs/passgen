/// @file bench.h
/// @brief What a benchmark is, for the benchmark tool.
#pragma once
#include <passgen/container/hashmap.h>
#include <stdbool.h>

/// A benchmark, which is run many times to measure how long one run (an
/// iteration) takes.
typedef struct bench {
    /// Name of the benchmark, which selects it on the command line.
    const char *name;
    /// What the benchmark does.
    const char *desc;
    /// What one iteration processes some of (such as bytes), for the
    /// throughput.
    const char *unit;
    /// Whether an iteration uses up its input.
    ///
    /// When this is set, every iteration gets a fresh input from `prepare`,
    /// which is not part of the time it takes. The result of the iteration
    /// is given to `cleanup`, which frees it.
    bool consumes;
    /// Whether the benchmark only runs when it is asked for by name, such as
    /// for one which takes long and is not meant to get faster.
    bool optional;
    /// Prepares the input of the benchmark, from the options given with `-o`.
    ///
    /// If this is not NULL, its result is given to every iteration. If it
    /// returns NULL, the benchmark is skipped, such as when a file it needs
    /// is not there (which it says why).
    void *(*prepare)(const passgen_hashmap *opts);
    /// Runs a single iteration of the benchmark, with the prepared input (or
    /// NULL).
    void *(*iterate)(void *data);
    /// Frees what an iteration returned, if it is not NULL. This is not part
    /// of the time it takes.
    void (*cleanup)(void *data);
    /// Frees the prepared input, for benchmarks which do not consume it.
    void (*release)(void *data);
    /// How many units one iteration processes, given the prepared input.
    /// Without it, it is one.
    double (*multiplier)(void *data);
} bench;

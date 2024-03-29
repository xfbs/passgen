/// @file newbench.h
/// @brief Benchmark framework header file
#pragma once
#include <stdbool.h>
#include <passgen/container/hashmap.h>

/// Benchmark definition
typedef struct bench {
    /// Name of benchmark
    const char *name;
    /// Description of benchmark
    const char *desc;
    /// Unit that this benchmark measures
    const char *unit;
    /// Benchmark consumes data
    ///
    /// When this is set to true, every iteration will use a
    /// fresh set of data generated by calling the prepare
    /// method.
    bool consumes;
    /// Prepare input for an iteration of the benchmark.
    ///
    /// If this function is not NULL, it is called before
    /// running the benchmark and the return value is fed as
    /// input to the benchmark.
    void *(*prepare)(const passgen_hashmap *opts);
    /// Run a single iteration of the benchmark.
    ///
    /// If the benchmark has a non-NULL prepare method, the
    /// return value of that return method is used as input to
    /// the benchmark. Otherwise, the input is NULL.
    void *(*iterate)(void *data);
    /// Cleanup the return value of a single iteration of the
    /// benchmark.
    void (*cleanup)(void *data);
    /// Cleanup a prepared input of the benchmark.
    void (*release)(void *data);

    /// Multiplier for this benchmark
    double (*multiplier)(void *data);
} bench;

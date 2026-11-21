/// @file bench.c
///
/// Runs the benchmarks. Each one is run in batches of iterations which take
/// long enough to be timed precisely, and the time of an iteration is
/// measured once per batch (a sample). This is repeated until the time for
/// the benchmark is used up, and what is reported is the median of the
/// samples, with how much they spread.
#include "bench.h"
#include "passgen/config.h"
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/// How long a batch of iterations has to take at least, in seconds. Timing
/// something shorter than this is not precise.
#define BATCH_TIME 1e-3

/// The most iterations which are prepared for one batch of a benchmark which
/// consumes its input, as they are all kept in memory.
#define BATCH_CONSUMES_MAX 64

typedef enum mode {
    MODE_BENCH,
    MODE_ONESHOT,
    MODE_LIST,
} mode;

typedef struct options {
    const bench **benches;
    bool *enabled;
    passgen_hashmap options;
    mode mode;
    double time;
    size_t samples;
    size_t iterations;
    bool tsv;
    const char *program;
    const char *oneshot;
} options;

/// Whether a benchmark was measured.
typedef enum outcome {
    OUTCOME_MEASURED,
    OUTCOME_SKIPPED,
    OUTCOME_OUT_OF_MEMORY,
} outcome;

/// What was measured for a benchmark.
typedef struct result {
    size_t samples;
    size_t batch;
    double median;
    double q1;
    double q3;
    double multiplier;
} result;

static int show_help(const options *options) {
    const char *prog = options->program;
    printf(
        "Usage: %s [-t TIME] [-i SAMPLES] [-o NAME=VALUE...] [--tsv] "
        "[NAME...]\n",
        prog);
    printf("       %s -s NAME [-i ITERATIONS] [-o NAME=VALUE...]\n", prog);
    printf("       %s -l\n", prog);
    printf(
        "Runs the benchmarks whose names contain one of the NAMEs, or all of "
        "them.\n\n");
    printf("OPTIONS\n");
    printf("    -l, --list\n");
    printf("        Lists the benchmarks.\n");
    printf("    -s, --oneshot NAME\n");
    printf(
        "        Runs the benchmark NAME once (or ITERATIONS times), "
        "without\n");
    printf(
        "        measuring it, such as to run it under perf or cachegrind.\n");
    printf("    -t, --time TIME\n");
    printf(
        "        How long to run each benchmark, such as 2, 2s, 500ms or "
        "1m.\n");
    printf("        Defaults to 1s.\n");
    printf("    -i, --iterations N\n");
    printf(
        "        The fewest samples to take of each benchmark, also if that\n");
    printf(
        "        takes longer than TIME. Defaults to 10. With --oneshot, "
        "how\n");
    printf("        often to run it, which defaults to 1.\n");
    printf("    -o, --option NAME=VALUE\n");
    printf(
        "        Sets the option NAME of the benchmarks, which can be given\n");
    printf("        more than once.\n");
    printf("    --tsv\n");
    printf(
        "        Prints the results as tab-separated values, with times in\n");
    printf("        seconds, for scripts/bench-compare.py.\n");
    printf("    -h, --help\n");
    printf("        Shows this help.\n");
    return EXIT_SUCCESS;
}

extern const bench stack_push;
extern const bench stack_pop;
extern const bench random_system_u8;
extern const bench random_system_u16;
extern const bench random_system_u32;
extern const bench random_system_u64;
extern const bench random_system_read;
extern const bench random_xorshift_u8;
extern const bench random_xorshift_u16;
extern const bench random_xorshift_u32;
extern const bench random_xorshift_u64;
extern const bench random_xorshift_read;
extern const bench random_zero_u8;
extern const bench random_zero_u16;
extern const bench random_zero_u32;
extern const bench random_zero_u64;
extern const bench random_zero_read;
extern const bench hashmap_insert;
extern const bench hashmap_lookup;
extern const bench token_parse;
extern const bench bench_passgen_parse;
extern const bench bench_utf8_decode;
extern const bench wordlist_load;
extern const bench markov_build_0;
extern const bench markov_build_1;
extern const bench markov_build_2;
extern const bench markov_build_3;
extern const bench markov_build_4;
extern const bench markov_build_5;
extern const bench markov_build_6;
extern const bench markov_build_7;
extern const bench markov_build_8;
extern const bench resolve_presets;
extern const bench generate_apple2;
extern const bench generate_apple2_entropy;
extern const bench generate_apple2_streams;
extern const bench generate_words;
extern const bench generate_words_entropy;
extern const bench generate_markov;
extern const bench generate_markov_entropy;

#ifdef PASSGEN_MONOCYPHER
extern const bench master_derive;
extern const bench random_chacha20_u8;
extern const bench random_chacha20_u16;
extern const bench random_chacha20_u32;
extern const bench random_chacha20_u64;
extern const bench random_chacha20_read;
#endif

static const bench *benches[] = {
    &stack_push,
    &stack_pop,
    &random_system_u8,
    &random_system_u16,
    &random_system_u32,
    &random_system_u64,
    &random_system_read,
    &random_xorshift_u8,
    &random_xorshift_u16,
    &random_xorshift_u32,
    &random_xorshift_u64,
    &random_xorshift_read,
    &random_zero_u8,
    &random_zero_u16,
    &random_zero_u32,
    &random_zero_u64,
    &random_zero_read,
#ifdef PASSGEN_MONOCYPHER
    &random_chacha20_u8,
    &random_chacha20_u16,
    &random_chacha20_u32,
    &random_chacha20_u64,
    &random_chacha20_read,
#endif
    &hashmap_insert,
    &hashmap_lookup,
    &token_parse,
    &bench_passgen_parse,
    &bench_utf8_decode,
    &wordlist_load,
    &markov_build_0,
    &markov_build_1,
    &markov_build_2,
    &markov_build_3,
    &markov_build_4,
    &markov_build_5,
    &markov_build_6,
    &markov_build_7,
    &markov_build_8,
    &resolve_presets,
    &generate_apple2,
    &generate_apple2_entropy,
    &generate_apple2_streams,
    &generate_words,
    &generate_words_entropy,
    &generate_markov,
    &generate_markov_entropy,
#ifdef PASSGEN_MONOCYPHER
    &master_derive,
#endif
    // the end of the list.
    NULL,
};

/// Parses a duration, in seconds unless it has a unit (ns, us, ms, s, m or
/// h). Returns NAN if it is not one.
static double parse_time(const char *str) {
    char *end;
    errno = 0;
    double number = strtod(str, &end);
    if(errno || end == str || !isfinite(number) || number < 0) {
        return NAN;
    }

    static const struct {
        const char *suffix;
        double factor;
    } units[] = {
        {"", 1},
        {"s", 1},
        {"ms", 1e-3},
        {"us", 1e-6},
        {"ns", 1e-9},
        {"m", 60},
        {"h", 3600},
    };
    for(size_t i = 0; i < sizeof(units) / sizeof(units[0]); i++) {
        if(0 == strcmp(end, units[i].suffix)) {
            return number * units[i].factor;
        }
    }
    return NAN;
}

/// Parses a positive count. Returns zero if it is not one.
static size_t parse_count(const char *str) {
    char *end;
    errno = 0;
    unsigned long long count = strtoull(str, &end, 10);
    if(errno || end == str || *end || str[0] == '-') {
        return 0;
    }
    return count;
}

/// The time now, in seconds, from a clock which only goes forward.
static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/// Runs a batch of iterations, and returns how long it took. A benchmark
/// which consumes its input gets one prepared for every iteration. The
/// results are kept in `outputs` until the batch is done, so that freeing
/// them is not part of the time it took.
static double run_batch(
    const bench *bench,
    const options *options,
    void *data,
    void **outputs,
    size_t count) {
    if(bench->consumes) {
        for(size_t i = 0; i < count; i++) {
            outputs[i] = bench->prepare(&options->options);
        }
    }

    double start = now();
    for(size_t i = 0; i < count; i++) {
        outputs[i] = bench->iterate(bench->consumes ? outputs[i] : data);
    }
    double time = now() - start;

    for(size_t i = 0; i < count; i++) {
        if(outputs[i] && bench->cleanup) {
            bench->cleanup(outputs[i]);
        }
    }
    return time;
}

static int compare_double(const void *lhs, const void *rhs) {
    double a = *(const double *) lhs;
    double b = *(const double *) rhs;
    return (a > b) - (a < b);
}

/// The value at a fraction of the way through sorted samples, interpolating
/// between the two closest.
static double quantile(const double *sorted, size_t count, double fraction) {
    double position = fraction * (count - 1);
    size_t index = (size_t) position;
    if(index + 1 >= count) {
        return sorted[count - 1];
    }
    double weight = position - index;
    return sorted[index] * (1 - weight) + sorted[index + 1] * weight;
}

/// Measures a benchmark.
static outcome
measure(const bench *bench, const options *options, result *result) {
    void *data = NULL;
    if(bench->prepare && !bench->consumes) {
        data = bench->prepare(&options->options);
    }

    // how much an iteration processes is the same for every input, so it is
    // taken from the input of the first one, which is not measured.
    void *input = bench->consumes ? bench->prepare(&options->options) : data;
    if(bench->prepare && !input) {
        return OUTCOME_SKIPPED;
    }
    result->multiplier = bench->multiplier ? bench->multiplier(input) : 1;
    void *output = bench->iterate(input);
    if(output && bench->cleanup) {
        bench->cleanup(output);
    }

    size_t batch_max = bench->consumes ? BATCH_CONSUMES_MAX : SIZE_MAX;
    size_t outputs_count = 1;
    void **outputs = malloc(sizeof(void *));
    size_t samples_capacity = 64;
    double *samples = malloc(samples_capacity * sizeof(double));
    bool ok = outputs && samples;

    // the batch doubles until it takes long enough to be timed. this also
    // warms up caches and the branch predictor, so it is not measured.
    size_t batch = 1;
    while(ok) {
        if(batch > outputs_count) {
            void **resized = realloc(outputs, batch * sizeof(void *));
            if(!resized) {
                ok = false;
                break;
            }
            outputs = resized;
            outputs_count = batch;
        }
        double time = run_batch(bench, options, data, outputs, batch);
        if(time >= BATCH_TIME || batch >= batch_max) {
            break;
        }
        batch = batch * 2 > batch_max ? batch_max : batch * 2;
    }

    // the time for the benchmark starts once the batch size is known.
    double start = now();
    result->samples = 0;
    while(ok && (result->samples < options->samples ||
                 now() - start < options->time)) {
        if(result->samples == samples_capacity) {
            samples_capacity *= 2;
            double *resized =
                realloc(samples, samples_capacity * sizeof(double));
            if(!resized) {
                ok = false;
                break;
            }
            samples = resized;
        }
        double time = run_batch(bench, options, data, outputs, batch);
        samples[result->samples] = time / batch;
        result->samples += 1;
    }

    if(ok) {
        qsort(samples, result->samples, sizeof(double), compare_double);
        result->batch = batch;
        result->median = quantile(samples, result->samples, 0.5);
        result->q1 = quantile(samples, result->samples, 0.25);
        result->q3 = quantile(samples, result->samples, 0.75);
    }

    if(data && bench->release) {
        bench->release(data);
    }
    free(outputs);
    free(samples);
    return ok ? OUTCOME_MEASURED : OUTCOME_OUT_OF_MEMORY;
}

/// Formats a throughput with an SI prefix, such as 2.10 Mbytes/s.
static void
format_throughput(char *out, size_t size, double value, const char *unit) {
    static const char *prefixes[] = {"", "k", "M", "G", "T"};
    size_t prefix = 0;
    while(value >= 1000 &&
          prefix + 1 < sizeof(prefixes) / sizeof(prefixes[0])) {
        value /= 1000;
        prefix++;
    }
    snprintf(out, size, "%7.2f %s%s/s", value, prefixes[prefix], unit);
}

/// Formats a duration in seconds, in a unit which suits it.
static void format_time(char *out, size_t size, double time) {
    if(time < 1e-6) {
        snprintf(out, size, "%7.2f ns", time * 1e9);
    } else if(time < 1e-3) {
        snprintf(out, size, "%7.2f us", time * 1e6);
    } else if(time < 1) {
        snprintf(out, size, "%7.2f ms", time * 1e3);
    } else {
        snprintf(out, size, "%7.2f s ", time);
    }
}

static int passgen_bench_list(const options *options) {
    for(size_t i = 0; options->benches[i]; i++) {
        printf(
            "%-24s %s\n",
            options->benches[i]->name,
            options->benches[i]->desc);
    }
    return EXIT_SUCCESS;
}

static int passgen_bench_run(const options *options) {
    if(options->tsv) {
        printf("name\tunit\tmultiplier\tsamples\tbatch\tmedian\tq1\tq3\n");
    } else {
        printf(
            "%-24s %21s %12s %8s %8s\n",
            "benchmark",
            "throughput",
            "iteration",
            "spread",
            "samples");
    }
    fflush(stdout);

    for(size_t b = 0; options->benches[b]; b++) {
        if(!options->enabled[b]) {
            continue;
        }
        const bench *bench = options->benches[b];

        result result;
        outcome outcome = measure(bench, options, &result);
        if(outcome == OUTCOME_OUT_OF_MEMORY) {
            fprintf(stderr, "error: out of memory running %s\n", bench->name);
            return EXIT_FAILURE;
        }
        if(outcome == OUTCOME_SKIPPED) {
            if(!options->tsv) {
                printf("%-24s %21s\n", bench->name, "skipped");
            }
            continue;
        }

        if(options->tsv) {
            printf(
                "%s\t%s\t%.17g\t%zu\t%zu\t%.6e\t%.6e\t%.6e\n",
                bench->name,
                bench->unit,
                result.multiplier,
                result.samples,
                result.batch,
                result.median,
                result.q1,
                result.q3);
        } else {
            // the spread is half of the range the middle half of the samples
            // are in, relative to the median.
            char throughput[64];
            char iteration[32];
            format_throughput(
                throughput,
                sizeof(throughput),
                result.multiplier / result.median,
                bench->unit);
            format_time(iteration, sizeof(iteration), result.median);
            printf(
                "%-24s %21s %12s %7.1f%% %8zu\n",
                bench->name,
                throughput,
                iteration,
                100 * (result.q3 - result.q1) / 2 / result.median,
                result.samples);
        }
        fflush(stdout);
    }

    return EXIT_SUCCESS;
}

static int passgen_bench_oneshot(const options *options) {
    const bench *bench = NULL;
    for(size_t b = 0; options->benches[b]; b++) {
        if(0 == strcmp(options->oneshot, options->benches[b]->name)) {
            bench = options->benches[b];
        }
    }

    if(!bench) {
        fprintf(stderr, "error: there is no benchmark %s\n", options->oneshot);
        return EXIT_FAILURE;
    }

    void *data = NULL;
    if(bench->prepare && !bench->consumes) {
        data = bench->prepare(&options->options);
        if(!data) {
            return EXIT_FAILURE;
        }
    }

    for(size_t i = 0; i < options->iterations; i++) {
        void *input = data;
        if(bench->consumes) {
            input = bench->prepare(&options->options);
            if(!input) {
                return EXIT_FAILURE;
            }
        }
        void *output = bench->iterate(input);
        if(output && bench->cleanup) {
            bench->cleanup(output);
        }
    }

    if(data && bench->release) {
        bench->release(data);
    }

    return EXIT_SUCCESS;
}

// Adds an option given as `name=value`, which is split in place.
static bool add_option(options *options, char *arg) {
    char *value = strchr(arg, '=');
    if(!value) {
        fprintf(stderr, "error: option %s has to be NAME=VALUE\n", arg);
        return false;
    }
    *value = 0;
    value++;
    passgen_hashmap_insert(&options->options, arg, value, NULL);
    return true;
}

enum {
    OPTION_TSV = 256,
};

int main(int argc, char *argv[]) {
    const char *short_opts = "o:t:i:s:hl";
    static struct option long_opts[] = {
        {"list", no_argument, NULL, 'l'},
        {"oneshot", required_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {"time", required_argument, NULL, 't'},
        {"option", required_argument, NULL, 'o'},
        {"iterations", required_argument, NULL, 'i'},
        {"tsv", no_argument, NULL, OPTION_TSV},
        {NULL, no_argument, NULL, 0}};

    size_t count = 0;
    while(benches[count]) count++;

    options options = {
        .benches = benches,
        .enabled = calloc(count, sizeof(bool)),
        .mode = MODE_BENCH,
        .time = 1.0,
        .program = argv[0],
    };
    size_t iterations = 0;
    passgen_hashmap_init(&options.options, &passgen_hashmap_context_utf8);

    int ret = EXIT_FAILURE;
    while(true) {
        int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);
        if(opt < 0) break;

        switch(opt) {
            case 'h':
                ret = show_help(&options);
                goto done;
            case 's':
                options.mode = MODE_ONESHOT;
                options.oneshot = optarg;
                break;
            case 'i':
                iterations = parse_count(optarg);
                if(!iterations) {
                    fprintf(
                        stderr,
                        "error: %s is not a positive count\n",
                        optarg);
                    goto done;
                }
                break;
            case 't':
                options.time = parse_time(optarg);
                if(isnan(options.time)) {
                    fprintf(stderr, "error: %s is not a time\n", optarg);
                    goto done;
                }
                break;
            case 'l':
                options.mode = MODE_LIST;
                break;
            case 'o':
                if(!add_option(&options, optarg)) {
                    goto done;
                }
                break;
            case OPTION_TSV:
                options.tsv = true;
                break;
            case '?':
            default:
                goto done;
        }
    }

    options.samples = iterations ? iterations : 10;
    options.iterations = iterations ? iterations : 1;

    // the benchmarks whose names contain one of the arguments are run, or
    // all of them if there are none (except for the optional ones).
    for(size_t i = 0; i < count; i++) {
        options.enabled[i] = optind >= argc && !options.benches[i]->optional;
    }
    for(int arg = optind; arg < argc; arg++) {
        bool found = false;
        for(size_t i = 0; i < count; i++) {
            if(strstr(options.benches[i]->name, argv[arg])) {
                options.enabled[i] = true;
                found = true;
            }
        }
        if(!found) {
            fprintf(stderr, "error: no benchmark matches %s\n", argv[arg]);
            goto done;
        }
    }

    switch(options.mode) {
        case MODE_BENCH:
            ret = passgen_bench_run(&options);
            break;
        case MODE_ONESHOT:
            ret = passgen_bench_oneshot(&options);
            break;
        case MODE_LIST:
            ret = passgen_bench_list(&options);
            break;
    }

done:
    free(options.enabled);
    passgen_hashmap_free(&options.options);
    return ret;
}

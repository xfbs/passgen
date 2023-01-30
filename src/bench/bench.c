#include "bench.h"
#include <getopt.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX(a, b) ((a > b) ? (a) : (b))
#define MIN(a, b) ((a > b) ? (b) : (a))

typedef enum mode {
    MODE_BENCH,
    MODE_ONESHOT,
    MODE_VALGRIND,
    MODE_LIST,
} mode;

typedef struct options {
    const bench **benches;
    bool *enabled;
    passgen_hashmap options;
    mode mode;
    double time;
    uint64_t iter;
    bool verbose;
    size_t name_col;
    FILE *write;
    passgen_hashmap *checkpoint;
    const char *valgrind;
    const char *program;
    const char *oneshot;
} options;

static int show_help(const options *options) {
    const char *prog = options->program;
    printf(
        "Usage: %s [-t TIME] [-i ITER] [-o NAME=VALUE...] [NAME...]\n",
        prog);
    printf("       %s -s [-o NAME=VALUE...] NAME\n", prog);
    printf("       %s -l [NAME...]\n", prog);
    printf("Runs the benchmarks, generating a report.\n\n");
    printf("OPTIONS\n");
    printf("    -l, --list\n");
    printf("        List available benchmarks and quit.\n");
    printf("    -s, --oneshot\n");
    printf("        Run in oneshot mode. This will run the benchmark only once "
           "and will not\n");
    printf("        generate a report. Useful for running a benchmark under "
           "Cachegrind to\n");
    printf("        measure instructions and cache behaviour.\n");
    printf("    -t, --time TIME\n");
    printf("        Runs benchmark for specified TIME. This will increase the "
           "iteration count\n");
    printf("        until the benchmark has run for the specified time, in "
           "order to generate\n");
    printf("        more samples and have more confidence in the measured "
           "throughput. It will\n");
    printf("        never decrease the number of iterations. Can be set to "
           "zero to strictly\n");
    printf("        run benchmarks ITER times. Defaults to 1s.\n");
    printf("    -i, --iterations ITER\n");
    printf("        Run the benchmarks for at least the specified iteration "
           "count. Benchmarks\n");
    printf("        will run longer if the alotted TIME has not been used up. "
           "Has to be a\n");
    printf("        positive, non-zero integer. Defaults to 100.\n");
    printf("    -o, --option NAME=VALUE\n");
    printf("        Overrides the benchmark option NAME with VALUE. This "
           "option can be set\n");
    printf("        multiple times to override multiple options.\n");
    printf("    -w, --checkpoint FILE\n");
    printf("        Write a checkpoint to the given FILE. This will write out "
           "the current\n");
    printf("        benchmark stats to the file in a CSV format for detecting "
           "regressions.\n");
    printf("        Recommended to use with the --valgrind flag.\n");
    printf("    -r, --check FILE\n");
    printf("        Read current benchmark state from the given FILE and warn "
           "if a regression\n");
    printf("        is detected. It is recommended to only use checkpoints "
           "generated on the\n");
    printf("        same machine or to use the --valgrind flag to get "
           "machine-independent\n");
    printf("        data.\n");
    printf("    -c, --valgrind PATH\n");
    printf("        Use the valgrind tool at the given PATH to compute "
           "machine-indepdendent\n");
    printf("        benchmark statistics, used to detect regressions.\n");
    printf("    -v, --verbose\n");
    printf("        Enables verbose logging output.\n");
    return EXIT_SUCCESS;
}

static void *dummy_iterate(void *data) {
    (void) data;
    return NULL;
}

const bench dummy = {
    .name = "dummy_iteration",
    .desc = "Benchmark iterations.",
    .unit = "runs",
    .iterate = &dummy_iterate,
};

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

const bench *benches[] = {
    &dummy,
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
    &hashmap_insert,
    &hashmap_lookup,
    &token_parse,
    NULL,
};

static double parse_time(const char *str) {
    double number;
    char prefix[3];
    int ret =
        sscanf(str, "%lf%c%c%c", &number, &prefix[0], &prefix[1], &prefix[2]);

    if(ret < 2) {
        return NAN;
    }

    if(ret == 2) {
        switch(prefix[0]) {
            case 'd':
                number *= 24;
                // fall through
            case 'h':
                number *= 60;
                // fall through
            case 'm':
                number *= 60;
                // fall through
            case 's':
                return number;
            default:
                break;
        }
    }

    if(ret == 3 && prefix[1] == 's') {
        switch(prefix[0]) {
            case 'n':
                number /= 1000;
                // fall through
            case 'u':
                number /= 1000;
                // fall through
            case 'm':
                number /= 1000;
                return number;
            default:
                break;
        }
    }

    return NAN;
}

static size_t bench_len(const bench *benches[]) {
    size_t len = 0;
    while(benches[len]) len++;
    return len;
}

int passgen_bench_list(const options *options) {
    int name_col = options->name_col;

    size_t line_len = 80;
    char line[line_len + 1];
    for(size_t i = 0; options->benches[i]; i++) {
        const bench *bench = options->benches[i];
        size_t desc_offset = 0;
        size_t desc_len = strlen(bench->desc);

        size_t pos = 0;
        pos += sprintf(line, "%s", bench->name);
        pos += sprintf(line + pos, "%*c", (int) (name_col + 2 - pos), ' ');

        if(desc_len > (line_len - pos)) {
            while((desc_len - desc_offset) > (line_len - pos)) {
                size_t copy_bytes = MIN(line_len - pos, desc_len - desc_offset);
                memcpy(line + pos, bench->desc + desc_offset, copy_bytes);
                size_t break_pos = line_len - pos;
                while(break_pos && line[pos + break_pos] != ' ') break_pos--;
                while(break_pos && line[pos + break_pos] == ' ') break_pos--;
                break_pos++;
                line[pos + break_pos] = 0;
                printf("%s\n", line);
                desc_offset += break_pos;
                pos = sprintf(line, "%*c", name_col + 1, ' ');
            }

            if(desc_offset != desc_len) {
                sprintf(
                    line,
                    "%*c%s",
                    name_col + 1,
                    ' ',
                    bench->desc + desc_offset);
                printf("%s\n", line);
            }
        } else {
            printf("%s%s\n", line, bench->desc);
        }
    }

    return EXIT_SUCCESS;
}

int passgen_bench_run(const options *options) {
    setlocale(LC_NUMERIC, "");
    fprintf(
        stderr,
        "BENCHMARK NAME                 BENCHMARK RESULT         CHANGE\n");
    for(size_t b = 0; options->benches[b]; b++) {
        if(!options->enabled[b]) {
            continue;
        }

        const bench *bench = options->benches[b];

        if(options->verbose) {
            fprintf(stderr, "Running benchmark %s\n", bench->name);
        }

        void *data = NULL;
        if(bench->prepare) {
            data = bench->prepare(&options->options);
        }

        // warmup
        void *output = bench->iterate(data);
        if(output) {
            bench->cleanup(output);
        }

        double multiplier = 1.0;
        if(bench->multiplier) {
            multiplier = bench->multiplier(data);
        }

        double total_time = 0;
        clock_t start = clock();
        clock_t before, after;
        clock_t target = options->time * CLOCKS_PER_SEC;
        clock_t progress = 0;

        size_t padding = options->name_col - strlen(bench->name);

        size_t iterations = 0;
        for(; iterations < options->iter || (after - start) < target;
            iterations++) {
            if(bench->consumes) {
                data = bench->prepare(&options->options);
            }

            before = clock();
            void *output = bench->iterate(data);
            after = clock();

            total_time += (after - before) / (double) CLOCKS_PER_SEC;

            if(output) {
                bench->cleanup(output);
            }

            if(after >= progress) {
                fprintf(
                    stderr,
                    "\r%s:%*c %'24.2lf %s/s",
                    bench->name,
                    (int) padding,
                    ' ',
                    multiplier * iterations / total_time,
                    bench->unit);
                progress = after + CLOCKS_PER_SEC / 10;
            }
        }

        if(data && !bench->consumes) {
            bench->release(data);
        }

        printf(
            "\r%s:%*c %'24.2lf %s/s\n",
            bench->name,
            (int) padding,
            ' ',
            multiplier * iterations / total_time,
            bench->unit);
    }

    return EXIT_SUCCESS;
}

int passgen_bench_oneshot(const options *options) {
    const bench *bench = NULL;
    for(size_t b = 0; options->benches[b]; b++) {
        if(0 == strcmp(options->oneshot, options->benches[b]->name)) {
            bench = options->benches[b];
        }
    }

    if(!bench) {
        fprintf(stderr, "Error: benchmark '%s' not found\n", options->oneshot);
        return EXIT_FAILURE;
    }

    if(options->verbose) {
        fprintf(stderr, "Running benchmark %s\n", bench->name);
    }

    void *data = NULL;
    if(bench->prepare) {
        data = bench->prepare(&options->options);
    }

    for(size_t i = 0; i < options->iter; i++) {
        if(bench->consumes) {
            data = bench->prepare(&options->options);
        }

        void *output = bench->iterate(data);

        if(output) {
            bench->cleanup(output);
        }
    }

    if(data && !bench->consumes) {
        bench->release(data);
    }

    return EXIT_SUCCESS;
}

int passgen_bench_valgrind(const options *options) {
    return EXIT_SUCCESS;
}

void measure_name_col(options *options) {
    options->name_col = 1;
    for(size_t i = 0; options->benches[i]; i++) {
        const bench *bench = options->benches[i];
        size_t current = strlen(bench->name) + 1;
        options->name_col = MAX(options->name_col, current);
    }
}

void add_option(options *options, const char *arg) {
    char *value = strchr(arg, '=');
    *value = 0;
    value++;
    if(options->verbose) {
        fprintf(stderr, "Parsed option '%s' as '%s'\n", arg, value);
    }
    passgen_hashmap_insert(&options->options, arg, value);
}

int main(int argc, char *argv[]) {
    // define command-line options
    const char *short_opts = "o:t:i:w:r:c:s:hlv";
    static struct option long_opts[] = {
        {"list", no_argument, NULL, 'l'},
        {"oneshot", required_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"time", required_argument, NULL, 't'},
        {"option", required_argument, NULL, 'o'},
        {"iterations", required_argument, NULL, 'i'},
        {"valgrind", required_argument, NULL, 'c'},
        {"checkpoint", required_argument, NULL, 'w'},
        {"check", required_argument, NULL, 'r'},
        {NULL, no_argument, NULL, 0}};

    size_t benchlen = bench_len(benches);

    // parse command-line options
    options options = {
        .benches = benches,
        .enabled = calloc(benchlen, sizeof(bool)),
        .mode = MODE_BENCH,
        .iter = 100,
        .time = 1.0,
        .verbose = false,
        .program = argv[0],
    };

    passgen_hashmap_init(&options.options, &passgen_hashmap_context_default);
    measure_name_col(&options);

    while(true) {
        // parse next command-line option
        int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);

        // done parsing options
        if(opt < 0) break;

        switch(opt) {
            case 'h':
                return show_help(&options);
            case 's':
                options.mode = MODE_ONESHOT;
                options.oneshot = optarg;
                break;
            case 'i':
                options.iter = atoi(optarg);
                if(options.iter == 0) {
                    fprintf(stderr, "Error: iter '%s' is invalid\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 't':
                options.time = parse_time(optarg);
                if(isnan(options.time)) {
                    fprintf(stderr, "Error: time '%s' is invalid\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'v':
                options.verbose = true;
                break;
            case 'c':
                options.mode = MODE_VALGRIND;
                options.valgrind = optarg;
                break;
            case 'r':
                break;
            case 'w':
                options.write = fopen(optarg, "w");
                break;
            case 'l':
                options.mode = MODE_LIST;
                break;
            case 'o':
                add_option(&options, optarg);
                break;
            case '?':
            default:
                return EXIT_FAILURE;
        }
    }

    if(options.verbose) {
        fprintf(stderr, "Loaded %zu benches\n", benchlen);
    }

    // initialize enabled
    for(size_t i = 0; i < benchlen; i++) {
        options.enabled[i] = optind >= argc;
    }

    while(optind < argc) {
        for(size_t i = 0; i < benchlen; i++) {
            if(strstr(options.benches[i]->name, argv[optind])) {
                options.enabled[i] = true;
            }
        }
        optind++;
    }

    if(options.verbose) {
        if(options.mode == MODE_BENCH) {
            fprintf(
                stderr,
                "Mode: Benchmark (%zu it, %lfs\n",
                options.iter,
                options.time);
        }

        if(options.mode == MODE_ONESHOT) {
            fprintf(stderr, "Mode: Oneshot (%zu it)\n", options.iter);
        }
    }

    int ret;
    switch(options.mode) {
        case MODE_BENCH:
            ret = passgen_bench_run(&options);
            break;
        case MODE_ONESHOT:
            ret = passgen_bench_oneshot(&options);
            break;
        case MODE_VALGRIND:
            ret = passgen_bench_valgrind(&options);
            break;
        case MODE_LIST:
            ret = passgen_bench_list(&options);
            break;
    }

    free(options.enabled);
    passgen_hashmap_free(&options.options);

    return ret;
}

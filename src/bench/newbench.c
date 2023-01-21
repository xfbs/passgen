#include "newbench.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

static int show_help(const char *prog) {
    printf("Usage: %s [-t TIME] [-i ITER] [-o NAME=VALUE...] [NAME...]\n", prog);
    printf("       %s -s [-o NAME=VALUE...] NAME\n", prog);
    printf("       %s -l [NAME...]\n", prog);
    printf("Runs the benchmarks, generating a report.\n\n");
    printf("OPTIONS\n");
    printf("    -l, --list\n");
    printf("        List available benchmarks and quit.\n");
    printf("    -s, --oneshot\n");
    printf("        Run in oneshot mode. This will run the benchmark only once and will not\n");
    printf("        generate a report. Useful for running a benchmark under Cachegrind to\n");
    printf("        measure instructions and cache behaviour.\n");
    printf("    -t, --time TIME\n");
    printf("        Runs benchmark for specified TIME. This will increase the iteration count\n");
    printf("        until the benchmark has run for the specified time, in order to generate\n");
    printf("        more samples and have more confidence in the measured throughput. It will\n");
    printf("        never decrease the number of iterations. Can be set to zero to strictly\n");
    printf("        run benchmarks ITER times. Defaults to 1s.\n");
    printf("    -i, --iterations ITER\n");
    printf("        Run the benchmarks for at least the specified iteration count. Benchmarks\n");
    printf("        will run longer if the alotted TIME has not been used up. Has to be a\n");
    printf("        positive, non-zero integer. Defaults to 100.\n");
    printf("    -o, --option NAME=VALUE\n");
    printf("        Overrides the benchmark option NAME with VALUE. This option can be set\n");
    printf("        multiple times to override multiple options.\n");
    printf("    -v, --verbose\n");
    printf("        Enables verbose logging output.\n");
    return EXIT_SUCCESS;
}

typedef enum mode {
    MODE_BENCH,
    MODE_ONESHOT,
    MODE_LIST,
} mode;

const bench test1 = {
    .group = "bench",
    .name = "testlong1",
    .desc = "This is another description of a benchmark, this one running somewhat longer and needing to be line-broken. This is some filler text that is inserted to make this description super verbose.",
};

const bench test2 = {
    .group = "bench",
    .name = "test2",
    .desc = "This is an example description of a benchmark.",
};

const bench test3 = {
    .group = "stack",
    .name = "push_pop",
    .desc = "This is an example description of a benchmark.",
};

const bench *benches[] = {
    &test1,
    &test2,
    &test3,
    NULL,
};

typedef struct options {
    const bench **benches;
    mode mode;
    double time;
    uint64_t iter;
    bool verbose;
} options;

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
            case 'h':
                number *= 60;
            case 'm':
                number *= 60;
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
            case 'u':
                number /= 1000;
            case 'm':
                number /= 1000;
                return number;
            default:
                break;
        }
    }

    return NAN;
}

#define MAX(a, b) ((a > b) ? (a) : (b))
#define MIN(a, b) ((a > b) ? (b) : (a))

int passgen_bench_list(const options *options) {
    int name_col = 1;
    for(size_t i = 0; options->benches[i]; i++) {
        bench *bench = options->benches[i];
        int current = strlen(bench->name) + strlen(bench->group) + 1;
        name_col = MAX(name_col, current);
    }

    size_t line_len = 80;
    char line[line_len + 1];
    for(size_t i = 0; options->benches[i]; i++) {
        bench *bench = options->benches[i];
        size_t desc_offset = 0;
        size_t desc_len = strlen(bench->desc);

        size_t pos = 0;
        pos += sprintf(line, "%s:%s", bench->group, bench->name);
        pos += sprintf(line + pos, "%*c", name_col + 2 - pos, ' ');

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
                sprintf(line, "%*c%s", name_col + 1, ' ', bench->desc + desc_offset);
                printf("%s\n", line);
            }
        } else {
            printf("%s%s\n", line, bench->desc);
        }
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    // define command-line options
    const char *short_opts = "s:t:i:hlv";
    static struct option long_opts[] = {
        {"list", no_argument, NULL, 'l'},
        {"oneshot", no_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"time", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {NULL, no_argument, NULL, 0}};

    // parse command-line options
    options options = {
        .benches = benches,
        .mode = MODE_BENCH,
        .iter = 100,
        .time = 1.0,
        .verbose = false,
    };

    while(true) {
        // parse next command-line option
        int opt = getopt_long(
            argc,
            argv,
            short_opts,
            long_opts,
            NULL);

        // done parsing options
        if(opt < 0) break;

        switch(opt) {
            case 'h':
                return show_help(argv[0]);
            case 's':
                options.mode = MODE_ONESHOT;
                break;
            case 'i':
                options.iter = atoi(optarg);
                if(options.iter == 0) {
                    printf("Error: iter '%s' is invalid\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 't':
                options.time = parse_time(optarg);
                if(isnan(options.time)) {
                    printf("Error: time '%s' is invalid\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'v':
                options.verbose = true;
                break;
            case 'l':
                options.mode = MODE_LIST;
                break;
            case '?':
            default:
                return EXIT_FAILURE;
        }
    }

    if(options.verbose) {
        if(options.mode == MODE_BENCH) {
            fprintf(stderr, "Running benchmark\n");
            fprintf(stderr, "Iterations %zu\n", options.iter);
            fprintf(stderr, "Time %lfs\n", options.time);
        }
        if(options.mode == MODE_ONESHOT) {
            fprintf(stderr, "Running oneshot\n");
        }
    }

    switch(options.mode) {
        case MODE_BENCH:
            break;
        case MODE_ONESHOT:
            break;
        case MODE_LIST:
            passgen_bench_list(&options);
            break;
    }

    return EXIT_SUCCESS;
}

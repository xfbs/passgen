#include "tests.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

test_result test_ok = {.ok = true};

// check if a string has the given prefix
static inline bool strprefix(const char *prefix, const char *string) {
    for(size_t offset = 0; prefix[offset]; offset++) {
        if(prefix[offset] != string[offset]) {
            return false;
        }
    }

    return true;
}

// swap memory of a given size
static inline void memswap(void *a, void *b, size_t size) {
    char data[size];

    memcpy(data, a, size);
    memcpy(a, b, size);
    memcpy(b, data, size);
}

// shuffle the list of tests
static inline void shuffle(test_entry *tests) {
    for(size_t i = 0; i < tests_len - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (tests_len - i) + 1);
        memswap(&tests[i], &tests[j], sizeof(test_entry));
    }
}

static inline void filter(bool *enabled, const char *name) {
    for(size_t i = 0; i < tests_len; i++) {
        if(strprefix(name, tests[i].name)) {
            enabled[i] = true;
        }
    }
}

static void show_help(const char *name) {
    fprintf(
        stderr,
        "%s%s",
        name,
        " [OPTIONS] [FILTERS...]\n"
        "Runs unit tests for passgen and reports results.\n\n"
        "Options\n"
        "  -h, --help\n"
        "    Shows this help text\n"
        "  -v, --verbose\n"
        "    Shows more verbose information\n"
        "  -s, --shuffle\n"
        "    Randomises the order in which tests are run\n\n"
        "Filters\n"
        "  Match based on prefix. A filter such as 'random' would match any "
        "test "
        "case\n"
        "  beginning with random, such as random_init, random_u8, etc.\n");
}

int main(int argc, char *argv[]) {
    // initialise shitty pseudorandom number generator
    srand(time(NULL));
    int verbosity = isatty(fileno(stdout));

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"shuffle", no_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    while(1) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "vsh", long_options, &option_index);

        /* Detect the end of the options. */
        if(c == -1) break;

        switch(c) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if(long_options[option_index].flag != 0) break;
                printf("option %s", long_options[option_index].name);
                if(optarg) printf(" with arg %s", optarg);
                printf("\n");
                break;

            case 's':
                shuffle(tests);
                break;

            case 'v':
                verbosity = 2;
                break;

            case 'h':
                show_help(argv[0]);
                return EXIT_SUCCESS;

            case '?':
                show_help(argv[0]);
                return EXIT_FAILURE;

            default:
                abort();
        }
    }

    bool enabled_tests[tests_len];
    if(optind < argc) {
        memset(enabled_tests, 0, sizeof(enabled_tests));

        for(int i = optind; i < argc; i++) {
            filter(enabled_tests, argv[i]);
        }
    } else {
        memset(enabled_tests, 1, sizeof(enabled_tests));
    }

    // determine max test name width and count of enabled tests.
    int width = 20;
    size_t enabled_count = 0;
    for(size_t i = 0; i < tests_len; i++) {
        if(enabled_tests[i]) {
            enabled_count++;
            int len = strlen(tests[i].name);
            if(len > width) width = len;
        }
    }

    size_t failures = 0;
    size_t success = 0;
    for(size_t i = 0; tests[i].name; ++i) {
        if(enabled_tests[i]) {
            test_run r = {
                .entry = tests[i],
                .verbosity = verbosity,
                .number = success + failures,
                .amount = enabled_count};

            if(run(r, width)) {
                success += 1;
            } else {
                failures += 1;
            }
        }
    }

    if(success == enabled_count) {
        switch(verbosity) {
            case 0:
                printf("=> %zi/%zi tests passed.\n", success, enabled_count);
                break;
            case 1:
            case 2:
                printf(
                    "\033[1;34m=>\033[0m %zi/%zi tests "
                    "\033[32mpassed\033[0m.\n",
                    success,
                    enabled_count);
                break;
        }
    } else {
        switch(verbosity) {
            case 0:
                printf(
                    "=> %zi/%zi tests passed, %zi failures.\n",
                    success,
                    enabled_count,
                    failures);
                break;
            case 1:
            case 2:
                printf(
                    "\033[1;34m=>\033[0m %zi/%zi tests "
                    "\033[32mpassed\033[0m, "
                    "%zi \033[31mfailure\033[0m.\n",
                    success,
                    enabled_count,
                    failures);
                break;
        }
    }

    if(failures) {
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}

bool run(test_run test, int padding) {
    switch(test.verbosity) {
        case 0:
            // not a tty, regular
            printf(
                "[%i/%i] running %s\n",
                test.number + 1,
                test.amount,
                test.entry.name);
            fflush(stdout);
            break;
        case 1:
            // tty, regular
            printf(
                "\033[1K\r[%i/%i] running %s",
                test.number + 1,
                test.amount,
                test.entry.name);
            fflush(stdout);
            break;
        default:
            break;
    }

    clock_t before = clock();
    test_result ret = test.entry.func();
    clock_t total = clock() - before;

    double time = total / (CLOCKS_PER_SEC * 1.0);

    switch(test.verbosity) {
        case 0:
            if(!ret.ok) {
                printf(
                    "  %s failed at %s:%zi\n",
                    ret.assertion,
                    ret.func,
                    ret.line);
            } else if(time > 0.1) {
                printf("=> test run took too long (%4.3lfs).\n", time);
            }
            break;
        case 1:
            if(!ret.ok) {
                printf(
                    "\n  \033[0;33m%s\033[0m \033[0;31mfailed\033[0m at "
                    "%s:%zi\n",
                    ret.assertion,
                    ret.func,
                    ret.line);
            } else if(time > 0.1) {
                printf("\n  \033[0;33mtest run took %4.3lfs.\033[0m\n", time);
            } else if((test.number + 1) == test.amount) {
                printf("\n");
            }
            break;
        case 2:
            if(ret.ok) {
                printf(
                    "%-*s \033[0;32mpassed\033[0m in %s%4.3lfs\033[0m.\n",
                    padding,
                    test.entry.name,
                    (time > 0.1) ? "\033[0;33m" : "",
                    time);
            } else {
                printf(
                    "%-*s \033[0;31mfailed\033[0m in %4.3lfs.\n",
                    padding,
                    test.entry.name,
                    time);
                printf(
                    "    \033[0;31m%s\033[0m failed at %s:%zi\n",
                    ret.assertion,
                    ret.func,
                    ret.line);
            }
            break;
        default:
            break;
    }

    return ret.ok;
}

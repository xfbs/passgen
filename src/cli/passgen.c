#include "passgen.h"

#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PASSGEN_SECCOMP
#include <fcntl.h>
#include <seccomp.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "passgen/data/parser.h"
#include "passgen/data/pattern.h"
#include "passgen/data/token.h"
#include "passgen/generate.h"
#include "passgen/parser.h"
#include "passgen/passgen.h"
#include "passgen/random.h"
#include "passgen/token.h"
#include "passgen/utf8.h"
#include "passgen/version.h"
#include "passgen/wordlist.h"

#define UNUSED(x)              (void) x
#define bail(kind, data)       passgen_cli_bail(PASSGEN_ERROR_##kind, (void *) data)
#define strprefix(prefix, str) memcmp(prefix, str, strlen(prefix))

int passgen_cli_opts_random(passgen_cli_opts *opts, const char *random) {
    if(opts->random) {
        passgen_random_free(opts->random);
        opts->random = NULL;
    }

    // check if we should read randomness from this file
    if(strprefix("file:", random) == 0) {
        opts->random = passgen_random_new_path(&random[5]);
        return 0;
    }

    // check if we should use the xorshift PRNG with the given seed
    if(strprefix("xor:", random) == 0) {
        const char *seed_str = &random[4];
        uint64_t seed = atoll(seed_str);
        if(seed == 0) {
            printf(
                "\033[1;31merror\033[0m: invalid xorshift seed '%s'\n",
                seed_str);
            return 1;
        }
        opts->random = passgen_random_new_xorshift(seed);
        return 0;
    }

    // check if we should use the system default
    if(0 == strcmp(random, "system")) {
        opts->random = passgen_random_new();
        return 0;
    }

    printf(
        "\033[1;31merror\033[0m: unrecognized randomness definition: `%s`\n",
        random);
    return 1;
}

void passgen_cli_run(passgen_cli_opts opts) {
    struct passgen_pattern pattern;
    struct passgen_error error;
    int ret = passgen_parse(&pattern, &error, opts.pattern);
    if(ret != 0) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m parsing pattern: %s\n",
            error.message);
        fprintf(stderr, "\033[1;33mpattern\033[0m: %s\n", opts.pattern);
        fprintf(stderr, "         ");
        for(size_t i = 1; i < error.byte; i++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "\033[1;31m^\033[0m\n");
        return;
    }

    passgen_cli_generate(opts, &pattern);
    passgen_pattern_free(&pattern);
}

void passgen_cli_generate_normal(
    passgen_cli_opts opts,
    struct passgen_pattern *pattern) {
    // allocate some space for pass.
    // size_t pass_len = pattern_maxlen(pattern);
    size_t pass_len = 256;
    uint8_t *pass = malloc(pass_len + 1);
    if(!pass) {
        bail(ALLOC, NULL);
    }

    char sep = '\n';
    if(opts.null) {
        sep = '\0';
    }

    struct passgen_env env = {
        .find_complexity = opts.complexity,
        .wordlists = opts.wordlists,
        .random = opts.random,
    };

    for(size_t i = 0; i < opts.amount; ++i) {
        // get a NULL-terminated, random pass.
        size_t written = passgen_generate_fill_utf8(
            pattern,
            opts.random,
            &env,
            pass,
            pass_len);
        pass[written] = '\0';

        if(opts.complexity) {
            fprintf(
                stderr,
                "entropy: %lf bits\n",
                log(env.complexity) / log(2));
        }

        if(i == 0) {
            printf("%s", pass);
        } else {
            printf("%c%s", sep, pass);
        }
    }

    printf("\n");
    free(pass);
}

void passgen_cli_generate_json(
    passgen_cli_opts opts,
    struct passgen_pattern *pattern) {
    // allocate some space for pass.
    // size_t pass_len = pattern_maxlen(pattern);
    size_t pass_len = 256;
    uint8_t *pass = malloc(pass_len + 1);
    if(!pass) {
        bail(ALLOC, NULL);
    }

    struct passgen_env env = {
        .find_complexity = opts.complexity,
        .wordlists = opts.wordlists,
        .random = opts.random,
    };

    printf("[");
    for(size_t i = 0; i < opts.amount; ++i) {
        // get a NULL-terminated, random pass.
        size_t written = passgen_generate_fill_json_utf8(
            pattern,
            opts.random,
            &env,
            pass,
            pass_len);
        pass[written] = '\0';
        printf("%s{\"generated\":\"%s\"", (i == 0) ? "" : ",", pass);

        if(opts.complexity) {
            printf(",\"complexity\":%lf", log(env.complexity) / log(2));
        }

        printf("}");
    }
    printf("]\n");

    free(pass);
}

void passgen_cli_generate(
    passgen_cli_opts opts,
    struct passgen_pattern *pattern) {
    if(!opts.json) {
        passgen_cli_generate_normal(opts, pattern);
    } else {
        passgen_cli_generate_json(opts, pattern);
    }
}

int passgen_cli_opts_wordlist(passgen_cli_opts *opt, const char *input) {
    const char *colon = strstr(input, ":");
    if(!colon) {
        return 1;
    }
    size_t name_len = colon - input;
    if(name_len == 0) {
        return 1;
    }

    // copy name
    uint32_t *name = calloc(256, sizeof(int32_t));
    size_t name_len_out = 0;
    size_t input_pos = 0;
    int ret = passgen_utf8_decode(
        name,
        256,
        &name_len_out,
        NULL,
        (unsigned char *) input,
        name_len,
        &input_pos);
    assert(ret == 0);

    FILE *file = fopen(colon + 1, "r");
    if(!file) {
        return 1;
    }

    passgen_wordlist *wordlist = malloc(sizeof(passgen_wordlist));
    passgen_hashmap_insert(&opt->wordlists, name, wordlist);
    passgen_wordlist_load(wordlist, file);

    return 0;
}

int passgen_cli_opts_preset(passgen_cli_opts *opts, const char *arg) {
    size_t arg_len = strlen(arg);
    char *arg_copy = malloc(arg_len + 1);
    memcpy(arg_copy, arg, arg_len + 1);
    char *colon = strstr(arg_copy, ":");
    if(!colon) {
        printf(
            "Error: expected preset:value for preset definition `%s`\n",
            arg);
        return 1;
    }
    *colon = 0;
    passgen_hashmap_insert(&opts->presets, arg_copy, colon + 1);
    return 0;
}

void passgen_cli_opts_init(passgen_cli_opts *opts) {
    opts->pattern = NULL;
    opts->amount = 1;
    opts->depth = 100;
    opts->null = false;
    opts->complexity = false;
    opts->random = NULL;
    opts->json = false;
    passgen_hashmap_init(&opts->wordlists, &passgen_hashmap_context_unicode);
    passgen_hashmap_init(&opts->presets, &passgen_hashmap_context_default);

    // some defaults
    passgen_hashmap_insert(
        &opts->presets,
        "apple1",
        "([a-zA-Z0-9]{3}-){3}[a-zA-Z0-9]{3}");
    passgen_hashmap_insert(
        &opts->presets,
        "apple2",
        "([a-zA-Z0-9]{6}-){2}[a-zA-Z0-9]{6}");
    passgen_hashmap_insert(
        &opts->presets,
        "uuid",
        "[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}");
    passgen_hashmap_insert(&opts->presets, "edge", "[A-Za-f0-9:_-]{15}");
    passgen_hashmap_insert(&opts->presets, "firefox", "[a-zA-Z0-9]{15}");
}

int passgen_cli_opts_parse(passgen_cli_opts *opts, int argc, char *argv[]) {
    const char *short_opts = "a:p:P:d:w:r:czhv";
    const char *preset = NULL;

    static struct option long_opts[] = {
        {"amount", required_argument, NULL, 'a'},
        {"help", no_argument, NULL, 'h'},
        {"preset", required_argument, NULL, 'p'},
        {"depth", required_argument, NULL, 'd'},
        {"version", no_argument, NULL, 'v'},
        {"null", no_argument, NULL, 'z'},
        {"complexity", no_argument, NULL, 'c'},
        {"wordlist", required_argument, NULL, 'w'},
        {"random", required_argument, NULL, 'r'},
        {"define-preset", required_argument, NULL, 'P'},
        {"json", no_argument, NULL, 'j'},
        {NULL, no_argument, NULL, 0}};

    while(true) {
        int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);
        int ret = 0;

        if(opt < 0) break;

        switch(opt) {
            case 0:
                break;
            case 'h':
                bail(HELP, argv[0]);
                break;
            case 'a':
                opt = atoi(optarg);
                if(opt < 1) {
                    bail(ILLEGAL_AMOUNT, &opt);
                }
                opts->amount = opt;
                break;
            case 'p':
                preset = optarg;
                break;
            case 'd':
                opt = atoi(optarg);
                opts->depth = opt;
                break;
            case 'z':
                opts->null = true;
                break;
            case 'c':
                opts->complexity = true;
                break;
            case 'v':
                bail(VERSION, NULL);
                break;
            case 'w':
                ret = passgen_cli_opts_wordlist(opts, optarg);
                break;
            case 'r':
                ret = passgen_cli_opts_random(opts, optarg);
                break;
            case 'P':
                ret = passgen_cli_opts_preset(opts, optarg);
                break;
            case 'j':
                opts->json = true;
                break;
            case '?':
            default:
                fprintf(stderr, "Error unrecognised: %s\n", argv[optind]);
                bail(HELP, argv[0]);
                break;
        }

        if(ret != 0) {
            return ret;
        }
    }

    // can't have both a preset and a format at the same time.
    if(preset && opts->pattern) {
        bail(MULTIPLE_FORMATS, opts->pattern);
    }

    // if a preset was given, parse it.
    if(preset) {
        passgen_hashmap_entry *entry =
            passgen_hashmap_lookup(&opts->presets, preset);
        if(!entry) {
            printf("\033[1;31merror\033[0m: preset `%s` not found\n", preset);
            return 1;
        }
        opts->pattern = entry->value;
    }

    // parse a given format, making sure we don't have multiple."
    if(optind < argc) {
        if(opts->pattern || (argc - optind) > 1) {
            bail(MULTIPLE_FORMATS, (void *) argv[optind]);
        } else {
            opts->pattern = argv[optind];
        }
    }

    // if no format or preset was given, show help.
    if(!opts->pattern) {
        bail(HELP, argv[0]);
    }

    // fallback to system randomness if nothing else is defined
    if(!opts->random) {
        opts->random = passgen_random_new();
        assert(opts->random);
    }

    return 0;
}

void passgen_cli_usage(const char *executable) {
    fprintf(
        stderr,
        "passgen version %s\n"
        "Generate passwords from a regex-like pattern.\n"
        "Usage: %s [OPTIONS] [PATTERN]\n\n"
        "PATTERN is a regex-like string describing the password.\n"
        "    abc|def           Matches string 'abc' or 'def' (choice).\n"
        "    [a-cf]            Matches character 'a', 'b', 'c', and 'f' "
        "(range).\n"
        "    (abc)             Matches strings 'abc' (group).\n"
        "    [a-c]{2,3}        Matches between 2 and 3 repetition of element "
        "(repeat).\n"
        "\n"
        "OPTIONS\n"
        "    -a, --amount      Amount of passwords to generate\n"
        "    -c, --complexity  Output complexity for each password.\n"
        "    -h, --help        Show this help information\n"
        "    -j, --json        Output as JSON\n"
        "    -v, --version     Show the version of this build.\n"
        "    -p, --preset      Use the given preset.\n"
        "    -r, --random STR  Which source of randomness to use. Can be:\n"
        "                          file:/path/to/file, to use the file\n"
        "                          xor:1234, to use xorshift with the given "
        "seed.\n"
        "    -w, --wordlist NAME:PATH\n"
        "                      Load a wordlist. For example:\n"
        "                          german:/usr/share/dict/ngerman\n"
        "                          english:/usr/share/dict/british-english\n"
        "    -z, --null        Use NUL instead of newline to separate "
        "passes.\n"
        "\n"
        "PRESETS\n"
        "    apple1            Generate passwords like 'oKC-T37-Dew-Qyn'\n"
        "    apple2            Generate passwords like "
        "'mHXr4X-CiK4w6-hbjF7T'\n"
        "    firefox           Generate passwords like 'aKTKyS9pPgAQ8Oz'\n"
        "    uuid              Generate UUIDv4 like "
        "0072ca58-5966-497c-8657-a59fca93bf25\n",
        passgen_version_str(),
        executable);
}

void passgen_cli_show_version(void) {
    fprintf(stderr, "passgen, version %s\n", passgen_version_str());

    if(passgen_is_debug()) {
        fprintf(stderr, "debug build\n");
    } else {
        fprintf(stderr, "release build\n");
    }
}

void passgen_cli_bail(passgen_cli_error error, void *data) {
    switch(error) {
        case PASSGEN_ERROR_HELP:
            passgen_cli_usage(data);
            exit(0);
        case PASSGEN_ERROR_VERSION:
            passgen_cli_show_version();
            exit(0);
        case PASSGEN_ERROR_MULTIPLE_FORMATS:
            printf(
                "Error: multiple patterns specified (%s).\n",
                (const char *) data);
            exit(-2);
        case PASSGEN_ERROR_RANDOM_ALLOC:
            printf("Error: couldn't open random object.\n");
            exit(-3);
        case PASSGEN_ERROR_PATTERN_PARSE:
            // printf("Error: couldn't parse pattern '%s'.\n", (const char
            // *)data);
            exit(-4);
        case PASSGEN_ERROR_ILLEGAL_AMOUNT:
            printf("Error: illegal amount entered (%i).\n", *((int *) data));
            exit(-5);
        default:
            printf("Error: unknown error.\n");
            exit(-100);
    }
}

void passgen_cli_opts_free(passgen_cli_opts *opts) {
    passgen_random_free(opts->random);
    passgen_hashmap_free(&opts->presets);
    passgen_hashmap_foreach_value(
        &opts->wordlists,
        (void *) passgen_wordlist_free);
    passgen_hashmap_foreach_key(&opts->wordlists, free);
    passgen_hashmap_free(&opts->wordlists);
}

int passgen_cli_opts_config_load(passgen_cli_opts *opts, FILE *file) {
    // TODO: use something like passgen_buffer to do this
    char *config = malloc(10240);
    size_t read = fread(config, 1, 10240, file);
    config[read] = 0;
    int ret = passgen_cli_opts_config_parse(opts, config);
    free(config);
    return ret;
}

int passgen_cli_opts_config_parse(passgen_cli_opts *opts, char *data) {
    // TODO: this code is a mess.
    for(size_t pos = 0; data[pos]; pos++) {
        if(data[pos] == '#') {
            // skip this line, it's a comment
            while(data[pos] && data[pos] != '\n') pos++;
            continue;
        }
        if(data[pos] == '\n') {
            // skip this newline
            continue;
        }
        if(0 == strprefix("random", &data[pos])) {
            pos += 6;
            while(data[pos] && (data[pos] == ' ' || data[pos] == '\t')) pos++;
            size_t length = 0;
            while(data[pos + length] && data[pos + length] != '\n') length++;
            data[pos + length] = 0;
            passgen_cli_opts_random(opts, &data[pos]);
            pos += length;
            continue;
        }
        if(0 == strprefix("wordlist", &data[pos])) {
            pos += 8;
            while(data[pos] && (data[pos] == ' ' || data[pos] == '\t')) pos++;
            size_t length = 0;
            while(data[pos + length] && data[pos + length] != '\n') length++;
            data[pos + length] = 0;
            passgen_cli_opts_wordlist(opts, &data[pos]);
            pos += length;
            continue;
        }
        if(0 == strprefix("preset", &data[pos])) {
            pos += 6;
            while(data[pos] && (data[pos] == ' ' || data[pos] == '\t')) pos++;
            size_t length = 0;
            while(data[pos + length] && data[pos + length] != '\n') length++;
            data[pos + length] = 0;
            passgen_cli_opts_preset(opts, &data[pos]);
            pos += length;
            continue;
        }
    }
    return 0;
}

int passgen_cli_opts_config(passgen_cli_opts *opts) {
    int ret;

    ret = passgen_cli_opts_config_system(opts);
    if(ret != 0) {
        return ret;
    }

    ret = passgen_cli_opts_config_user(opts);
    if(ret != 0) {
        return ret;
    }

    return 0;
}

int passgen_cli_opts_config_system(passgen_cli_opts *opts) {
    FILE *file = fopen("/etc/passgen.conf", "r");
    if(file) {
        int ret = passgen_cli_opts_config_load(opts, file);
        fclose(file);
        return ret;
    } else {
        return 0;
    }
}

int passgen_cli_opts_config_user(passgen_cli_opts *opts) {
    char config_path[256];
    const char *xdg_config_path = getenv("XDG_CONFIG_PATH");
    if(xdg_config_path) {
        sprintf(config_path, "%s/passgen.conf", xdg_config_path);
    } else {
        const char *home = getenv("HOME");
        if(!home) {
            fprintf(stderr, "error: cannot locate home directory\n");
            return -1;
        }
        sprintf(config_path, "%s/.config/passgen.conf", home);
    }
    FILE *file = fopen(config_path, "r");
    if(file) {
        int ret = passgen_cli_opts_config_load(opts, file);
        fclose(file);
        return ret;
    } else {
        return 0;
    }
}

#ifdef PASSGEN_SECCOMP
void passgen_cli_seccomp_init() {
    scmp_filter_ctx ctx;
    ctx = seccomp_init(SCMP_ACT_KILL);

    // allow open syscalls, as long as they are read-only.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(open),
        1,
        SCMP_CMP(1, SCMP_CMP_EQ, O_RDONLY));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(openat),
        1,
        SCMP_CMP(2, SCMP_CMP_EQ, O_RDONLY));

    // allow reading stat of files
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(newfstatat), 0);

    // allow reading and seeking in files
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);

    // allow writing, but only to stdout or stderr.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(write),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 2));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(write),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 1));

    // allow closing, as long as the fd is not any of the standard streams.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(close),
        1,
        SCMP_CMP(0, SCMP_CMP_GT, 2));

    // allow reading random data
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getrandom), 0);

    // allow exiting
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);

    // allow allocating memory
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(mmap),
        3,
        SCMP_CMP(0, SCMP_CMP_EQ, NULL),
        SCMP_CMP(2, SCMP_CMP_EQ, PROT_READ | PROT_WRITE),
        SCMP_CMP(3, SCMP_CMP_EQ, MAP_PRIVATE | MAP_ANONYMOUS));
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mremap), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);

    // apply rules
    seccomp_load(ctx);
    seccomp_release(ctx);
}
#endif

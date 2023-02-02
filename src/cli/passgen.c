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

#include "passgen/generate.h"
#include "passgen/parser/parser.h"
#include "passgen/parser/token.h"
#include "passgen/passgen.h"
#include "passgen/pattern/pattern.h"
#include "passgen/util/random.h"
#include "passgen/try.h"
#include "passgen/util/utf8.h"
#include "passgen/version.h"
#include "passgen/wordlist.h"

#define UNUSED(x)              (void) x
#define strprefix(prefix, str) memcmp(prefix, str, strlen(prefix))

int passgen_cli_opts_random(passgen_cli_opts *opts, const char *random) {
    if(opts->env.random) {
        passgen_random_free(opts->env.random);
        opts->env.random = NULL;
    }

    opts->env.random = passgen_random_new(random);

    if(!opts->env.random) {
        printf(
            "\033[1;31merror\033[0m: invalid randomness source '%s'\n",
            random);
        return 1;
    }

    return 0;
}

int passgen_cli_run(passgen_cli_opts opts) {
    passgen_pattern pattern;
    passgen_error error;
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
        return PASSGEN_ERROR_PATTERN_PARSE;
    }

    ret = passgen_cli_generate(opts, &pattern);
    passgen_pattern_free(&pattern);
    return ret;
}

int passgen_cli_generate_normal(
    passgen_cli_opts opts,
    passgen_pattern *pattern) {
    // allocate some space for pass.
    // size_t pass_len = pattern_maxlen(pattern);
    size_t pass_len = 256;
    uint8_t *pass = malloc(pass_len + 1);
    if(!pass) {
        return passgen_cli_bail(PASSGEN_ERROR_ALLOC, NULL);
    }

    char sep = '\n';
    if(opts.null) {
        sep = '\0';
    }

    for(size_t i = 0; i < opts.amount; ++i) {
        // get a NULL-terminated, random pass.
        size_t written =
            passgen_generate_fill_utf8(pattern, &opts.env, pass, pass_len);
        pass[written] = '\0';

        if(opts.env.entropy) {
            fprintf(
                stderr,
                "entropy: %.2lf bits\n",
                log(opts.env.entropy) / log(2));
        }

        if(i == 0) {
            printf("%s", pass);
        } else {
            printf("%c%s", sep, pass);
        }
    }

    printf("\n");
    free(pass);

    return 0;
}

int passgen_cli_generate_json(
    passgen_cli_opts opts,
    passgen_pattern *pattern) {
    // allocate some space for pass.
    // size_t pass_len = pattern_maxlen(pattern);
    size_t pass_len = 256;
    uint8_t *pass = malloc(pass_len + 1);
    if(!pass) {
        return passgen_cli_bail(PASSGEN_ERROR_ALLOC, NULL);
    }

    printf("[");
    for(size_t i = 0; i < opts.amount; ++i) {
        // get a NULL-terminated, random pass.
        size_t written =
            passgen_generate_fill_json_utf8(pattern, &opts.env, pass, pass_len);
        pass[written] = '\0';
        printf("%s{\"output\":\"%s\"", (i == 0) ? "" : ",", pass);

        if(opts.env.entropy) {
            printf(",\"entropy\":%lf", log(opts.env.entropy) / log(2));
        }

        printf("}");
    }
    printf("]\n");

    free(pass);
    return 0;
}

int passgen_cli_generate(
    passgen_cli_opts opts,
    passgen_pattern *pattern) {
    if(!opts.json) {
        return passgen_cli_generate_normal(opts, pattern);
    } else {
        return passgen_cli_generate_json(opts, pattern);
    }
}

int passgen_cli_opts_wordlist(passgen_cli_opts *opt, const char *input) {
    char *colon = strstr(input, ":");
    if(!colon) {
        return 1;
    }
    size_t offset = colon - input;

    char *name = malloc(offset + 1);
    memcpy(name, input, offset);
    name[offset] = 0;

    FILE *file = fopen(colon + 1, "r");
    if(!file) {
        return 1;
    }

    return passgen_env_wordlist_add(&opt->env, name, file, opt->markov_length);
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
    opts->json = false;
    opts->markov_length = 3;
    passgen_hashmap_init(&opts->presets, &passgen_hashmap_context_utf8);
    passgen_env_init(&opts->env, NULL);

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
    passgen_hashmap_insert(&opts->presets, "edge", "[A-Za-f0-9:_\\-]{15}");
    passgen_hashmap_insert(&opts->presets, "firefox", "[a-zA-Z0-9]{15}");
    passgen_hashmap_insert(
        &opts->presets,
        "proquint16",
        "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfghjklmnprstvz]["
        "aiou]){2}[bdfghjklmnprstvz]){0}");
    passgen_hashmap_insert(
        &opts->presets,
        "proquint32",
        "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfghjklmnprstvz]["
        "aiou]){2}[bdfghjklmnprstvz]){1}");
    passgen_hashmap_insert(
        &opts->presets,
        "proquint48",
        "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfghjklmnprstvz]["
        "aiou]){2}[bdfghjklmnprstvz]){2}");
    passgen_hashmap_insert(
        &opts->presets,
        "proquint64",
        "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfghjklmnprstvz]["
        "aiou]){2}[bdfghjklmnprstvz]){3}");
    passgen_hashmap_insert(
        &opts->presets,
        "proquint80",
        "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfghjklmnprstvz]["
        "aiou]){2}[bdfghjklmnprstvz]){4}");
    passgen_hashmap_insert(
        &opts->presets,
        "proquint96",
        "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfghjklmnprstvz]["
        "aiou]){2}[bdfghjklmnprstvz]){5}");
    passgen_hashmap_insert(
        &opts->presets,
        "proquint112",
        "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfghjklmnprstvz]["
        "aiou]){2}[bdfghjklmnprstvz]){6}");
    passgen_hashmap_insert(
        &opts->presets,
        "proquint128",
        "([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfghjklmnprstvz]["
        "aiou]){2}[bdfghjklmnprstvz]){7}");
}

int passgen_cli_preset_show(void *data, passgen_hashmap_entry *entry) {
    (void) data;
    printf(
        "preset %s %s\n",
        (const char *) entry->key,
        (const char *) entry->value);
    return 0;
}

void passgen_cli_presets_list(const passgen_cli_opts *opts) {
    passgen_hashmap_foreach(
        &opts->presets,
        (void *) opts,
        passgen_cli_preset_show);
}

int passgen_cli_opts_parse(passgen_cli_opts *opts, int argc, char *argv[]) {
    const char *short_opts = "a:p:P:d:w:r:m:ezhvjl";
    const char *preset = NULL;

    static struct option long_opts[] = {
        {"amount", required_argument, NULL, 'a'},
        {"help", no_argument, NULL, 'h'},
        {"preset", required_argument, NULL, 'p'},
        {"depth", required_argument, NULL, 'd'},
        {"version", no_argument, NULL, 'v'},
        {"null", no_argument, NULL, 'z'},
        {"entropy", no_argument, NULL, 'e'},
        {"wordlist", required_argument, NULL, 'w'},
        {"random", required_argument, NULL, 'r'},
        {"list", no_argument, NULL, 'l'},
        {"define-preset", required_argument, NULL, 'P'},
        {"json", no_argument, NULL, 'j'},
        {"markov-length", required_argument, NULL, 'm'},
        {NULL, no_argument, NULL, 0}};

    while(true) {
        int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);

        if(opt < 0) break;

        switch(opt) {
            case 0:
                break;
            case 'h':
                return PASSGEN_SHOW_HELP;
            case 'a':
                opt = atoi(optarg);
                if(opt < 1) {
                    return passgen_cli_bail(PASSGEN_ERROR_ILLEGAL_AMOUNT, &opt);
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
            case 'e':
                opts->env.find_entropy = true;
                break;
            case 'v':
                return PASSGEN_SHOW_VERSION;
            case 'l':
                passgen_cli_presets_list(opts);
                exit(0);
            case 'w':
                try(passgen_cli_opts_wordlist(opts, optarg));
                break;
            case 'm':
                opt = atoi(optarg);
                if(opt < 1) {
                    return passgen_cli_bail(
                        PASSGEN_ERROR_ILLEGAL_MARKOV_LENGTH,
                        &opt);
                }
                opts->markov_length = opt;
                break;
            case 'r':
                try(passgen_cli_opts_random(opts, optarg));
                break;
            case 'P':
                try(passgen_cli_opts_preset(opts, optarg));
                break;
            case 'j':
                opts->json = true;
                break;
            case '?':
            default:
                return passgen_cli_bail(
                    PASSGEN_ERROR_UNRECOGNIZED,
                    argv[optind]);
        }
    }

    // can't have both a preset and a format at the same time.
    if(preset && opts->pattern) {
        return passgen_cli_bail(PASSGEN_ERROR_MULTIPLE_FORMATS, opts->pattern);
    }

    // if a preset was given, parse it.
    if(preset) {
        passgen_hashmap_entry *entry =
            passgen_hashmap_lookup(&opts->presets, preset);
        if(!entry) {
            return passgen_cli_bail(PASSGEN_ERROR_PRESET_NOT_FOUND, preset);
        }
        opts->pattern = entry->value;
    }

    // parse a given format, making sure we don't have multiple."
    if(optind < argc) {
        if(opts->pattern || (argc - optind) > 1) {
            return passgen_cli_bail(
                PASSGEN_ERROR_MULTIPLE_FORMATS,
                argv[optind]);
        } else {
            opts->pattern = argv[optind];
        }
    }

    // if no format or preset was given, show help.
    if(!opts->pattern) {
        return PASSGEN_SHOW_HELP;
    }

    // fallback to system randomness if nothing else is defined
    if(!opts->env.random) {
        opts->env.random = passgen_random_new(NULL);
        assert(opts->env.random);
    }

    return 0;
}

void passgen_cli_usage(const char *executable) {
    passgen_version version = passgen_version_get();
    fprintf(
        stderr,
        "passgen version v%hu.%hu.%hu\n",
        version.major,
        version.minor,
        version.patch);
    fprintf(
        stderr,
        "Generate random sequences from a regex-like pattern.\n"
        "Usage: %s [OPTIONS] [PATTERN]\n\n"
        "PATTERN is a regex-like string describing the password.\n"
        "    a                  Literal: outputs character 'a'\n"
        "    abc|def            Choice: outputs string 'abc' or 'def'\n"
        "    [a-z0-9:]          Range: outputs a lowercase letter, number, or "
        "':'\n"
        "    (abc)              Group: outputs 'abc'\n"
        "    a{2}               Repeat: outputs 'aa'\n"
        "\n"
        "OPTIONS\n"
        "    -a, --amount       Amount of passwords to generate\n"
        "    -e, --entropy      Compute entropy for each password in bits.\n"
        "    -h, --help         Show this help information\n"
        "    -j, --json         Output as JSON\n"
        "    -v, --version      Show the version of this build\n"
        "    -p, --preset NAME  Use the given preset\n"
        "    -l, --list         List loaded presets\n"
        "    -r, --random RAND  Source of randomness to use\n"
        "    -w, --wordlist NAME:PATH\n"
        "                       Load wordlist NAME from PATH\n"
        "    -m, --markov-length LENGTH\n"
        "                       Specify markov chain length for next wordlist\n"
        "    -z, --null         Use NUL instead of newline to separate "
        "outputs\n"
        "\n"
        "PRESETS\n"
        "    apple1            Generate passwords like 'oKC-T37-Dew-Qyn'\n"
        "    apple2            Generate passwords like "
        "'mHXr4X-CiK4w6-hbjF7T'\n"
        "    firefox           Generate passwords like 'aKTKyS9pPgAQ8Oz'\n"
        "    uuid              Generate UUIDv4 like "
        "0072ca58-5966-497c-8657-a59fca93bf25\n",
        executable);
}

void passgen_cli_show_version(void) {
    passgen_version version = passgen_version_get();
    fprintf(
        stderr,
        "passgen version v%hu.%hu.%hu (build %s)\n",
        version.major,
        version.minor,
        version.patch,
        version.build);

    if(passgen_is_debug()) {
        fprintf(stderr, "debug build\n");
    } else {
        fprintf(stderr, "release build\n");
    }
}

int passgen_cli_bail(passgen_cli_error error, const void *data) {
    switch(error) {
        case PASSGEN_ERROR_MULTIPLE_FORMATS:
            printf(
                "error: multiple patterns specified (%s).\n",
                (const char *) data);
            break;
        case PASSGEN_ERROR_RANDOM_ALLOC:
            printf("error: couldn't open random object.\n");
            break;
        case PASSGEN_ERROR_PATTERN_PARSE:
            break;
        case PASSGEN_ERROR_ILLEGAL_AMOUNT:
            printf(
                "\033[1;31merror\033[0m: illegal amount entered (%i)\n",
                *((const int *) data));
            break;
        case PASSGEN_ERROR_ILLEGAL_MARKOV_LENGTH:
            printf(
                "\033[1;31merror\033[0m: illegal markov chain length entered "
                "(%i)\n",
                *((const int *) data));
            break;
        case PASSGEN_ERROR_UNRECOGNIZED:
            break;
        case PASSGEN_ERROR_PRESET_NOT_FOUND:
            printf(
                "\033[1;31merror\033[0m: preset `%s` not found\n",
                (const char *) data);
            break;
        default:
            printf("error: unknown error.\n");
            break;
    }

    return error;
}

void passgen_cli_opts_free(passgen_cli_opts *opts) {
    passgen_hashmap_free(&opts->presets);
    passgen_env_free(&opts->env);
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
    try(passgen_cli_opts_config_system(opts));
    try(passgen_cli_opts_config_user(opts));
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
            fprintf(stderr, "warning: cannot locate home directory\n");
            return 0;
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

/// @file options.c
///
/// The options of the command-line tool: parsing them, the help, and what is
/// defined with them (presets, wordlists, the source of randomness).
#include "passgen.h"
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Prints that a randomness source cannot be used. A description which may
/// hold a secret (see @ref passgen_random_desc_secret) is not printed in full,
/// so that the secret does not end up in a terminal or a log: only its kind,
/// which is what comes before the first colon.
static void passgen_cli_random_error(const char *random) {
    const char *colon = strchr(random, ':');
    if(colon && passgen_random_desc_secret(random)) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: cannot use randomness source '%.*s:...'\n",
            (int) (colon - random),
            random);
    } else {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: cannot use randomness source '%s'\n",
            random);
    }
}

int passgen_cli_opts_random(passgen_cli_opts *opts, const char *random) {
    if(opts->random) {
        passgen_random_free(opts->random);
        opts->random = NULL;
    }

    opts->random = passgen_random_open(NULL, random);

    if(!opts->random) {
        passgen_cli_random_error(random);
        return PASSGEN_CLI_ERROR_RANDOM_ALLOC;
    }

    return 0;
}

int passgen_cli_opts_wordlist(passgen_cli_opts *opt, const char *input) {
    const char *colon = strstr(input, ":");
    if(!colon) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: expected NAME:PATH for wordlist "
            "definition `%s`\n",
            input);
        return PASSGEN_CLI_ERROR_WORDLIST;
    }
    size_t offset = colon - input;

    char *name = malloc(offset + 1);
    if(!name) {
        return PASSGEN_CLI_ERROR_ALLOC;
    }
    memcpy(name, input, offset);
    name[offset] = 0;

    // the file is only opened when a pattern uses the wordlist.
    passgen_status status =
        passgen_env_wordlist_add_path(opt->env, name, colon + 1, NULL);
    free(name);

    return status == PASSGEN_OK ? 0 : PASSGEN_CLI_ERROR_ALLOC;
}

int passgen_cli_opts_preset(passgen_cli_opts *opts, const char *arg) {
    const char *colon = strchr(arg, ':');
    if(!colon) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: expected NAME:PATTERN for preset "
            "definition `%s`\n",
            arg);
        return PASSGEN_CLI_ERROR_PRESET_NOT_FOUND;
    }

    size_t offset = colon - arg;
    char *name = malloc(offset + 1);
    if(!name) {
        return PASSGEN_CLI_ERROR_ALLOC;
    }
    memcpy(name, arg, offset);
    name[offset] = 0;

    // the environment copies the name and the pattern. the pattern is only
    // parsed once something uses it.
    passgen_status status = passgen_env_preset_add(opts->env, name, colon + 1);
    free(name);

    return status == PASSGEN_OK ? 0 : PASSGEN_CLI_ERROR_ALLOC;
}

void passgen_cli_opts_init(passgen_cli_opts *opts) {
    opts->pattern = NULL;
    opts->preset = NULL;
    opts->list = false;
    opts->definitions = NULL;
    opts->definitions_len = 0;
    opts->configs = NULL;
    opts->configs_len = 0;
    opts->amount = 1;
    opts->offset = 0;
    opts->offset_given = false;
    opts->null = false;
    opts->json = false;
    opts->entropy = false;
#ifdef PASSGEN_MONOCYPHER
    opts->master = NULL;
    opts->master_domain = NULL;
    opts->master_user = NULL;
    opts->master_setup = NULL;
    opts->master_setup_given = false;
    opts->master_setup_level = 0;
    opts->master_setup_seed = false;
    opts->master_setup_version = false;
#endif
    opts->masters = NULL;
    opts->masters_len = 0;
    opts->logins = NULL;
    opts->logins_len = 0;
    opts->random = NULL;
    opts->env = passgen_env_new(NULL);

    // some defaults. these are defined the same way as presets from the
    // command line or configuration, so that every preset owns its memory.
    static const char *const defaults[] = {
        "apple1:([a-zA-Z0-9]{3}-){3}[a-zA-Z0-9]{3}",
        "apple2:([a-zA-Z0-9]{6}-){2}[a-zA-Z0-9]{6}",
        "uuid:[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-"
        "f]{12}",
        "edge:[A-Za-f0-9:_\\-]{15}",
        "firefox:[a-zA-Z0-9]{15}",
        "proquint16:([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfg"
        "hjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]){0}",
        "proquint32:([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfg"
        "hjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]){1}",
        "proquint48:([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfg"
        "hjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]){2}",
        "proquint64:([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfg"
        "hjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]){3}",
        "proquint80:([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfg"
        "hjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]){4}",
        "proquint96:([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdfg"
        "hjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]){5}",
        "proquint112:([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdf"
        "ghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]){6}",
        "proquint128:([bdfghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz](-([bdf"
        "ghjklmnprstvz][aiou]){2}[bdfghjklmnprstvz]){7}",
    };

    for(size_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {
        passgen_cli_opts_preset(opts, defaults[i]);
    }
}

// the options for master passphrase mode only exist when it does, which is
// when building with monocypher.
#ifdef PASSGEN_MONOCYPHER
#define PASSGEN_CLI_MASTER_OPTS "d:m:u:"

#else
#define PASSGEN_CLI_MASTER_OPTS ""
#endif

// options which only have a long form.
enum {
    PASSGEN_CLI_OPT_OFFSET = 256,
    PASSGEN_CLI_OPT_MASTER_SETUP,
    PASSGEN_CLI_OPT_MASTER_LEVEL,
    PASSGEN_CLI_OPT_MASTER_SEED,
    PASSGEN_CLI_OPT_MASTER_VERSION,
};

int passgen_cli_opts_parse(passgen_cli_opts *opts, int argc, char *argv[]) {
    const char *short_opts = "a:c:p:P:w:r:S:ezhvjl" PASSGEN_CLI_MASTER_OPTS;
    // whether the randomness source was set on the command line.
    bool random_given = false;

    // there cannot be more of these than there are arguments.
    opts->definitions = calloc(argc, sizeof(passgen_cli_definition));
    opts->configs = calloc(argc, sizeof(const char *));
    if(!opts->definitions || !opts->configs) {
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }

    static struct option long_opts[] = {
        {"amount", required_argument, NULL, 'a'},
        {"offset", required_argument, NULL, PASSGEN_CLI_OPT_OFFSET},
        {"config", required_argument, NULL, 'c'},
        {"help", no_argument, NULL, 'h'},
        {"preset", required_argument, NULL, 'p'},
        {"steps", required_argument, NULL, 'S'},
        {"version", no_argument, NULL, 'v'},
        {"null", no_argument, NULL, 'z'},
        {"entropy", no_argument, NULL, 'e'},
        {"wordlist", required_argument, NULL, 'w'},
        {"random", required_argument, NULL, 'r'},
        {"list", no_argument, NULL, 'l'},
        {"define-preset", required_argument, NULL, 'P'},
        {"json", no_argument, NULL, 'j'},
#ifdef PASSGEN_MONOCYPHER
        {"master", required_argument, NULL, 'm'},
        {"domain", required_argument, NULL, 'd'},
        {"user", required_argument, NULL, 'u'},
        {"master-setup", optional_argument, NULL, PASSGEN_CLI_OPT_MASTER_SETUP},
        {"master-level", required_argument, NULL, PASSGEN_CLI_OPT_MASTER_LEVEL},
        {"master-seed", no_argument, NULL, PASSGEN_CLI_OPT_MASTER_SEED},
        {"master-version",
         required_argument,
         NULL,
         PASSGEN_CLI_OPT_MASTER_VERSION},
#endif
        {NULL, no_argument, NULL, 0}};

    while(true) {
        int opt = getopt_long(argc, argv, short_opts, long_opts, NULL);

        if(opt < 0) break;

        switch(opt) {
            case 0:
                break;
            case 'h':
                return PASSGEN_CLI_SHOW_HELP;
            case 'a':
                opt = atoi(optarg);
                if(opt < 1) {
                    return passgen_cli_bail(
                        PASSGEN_CLI_ERROR_ILLEGAL_AMOUNT,
                        &opt);
                }
                opts->amount = opt;
                break;
#ifdef PASSGEN_MONOCYPHER
            case 'm':
                opts->master = optarg;
                break;
            case 'd':
                opts->master_domain = optarg;
                break;
            case 'u':
                opts->master_user = optarg;
                break;
            case PASSGEN_CLI_OPT_MASTER_SETUP:
                // as `--master-setup=name`. as `--master-setup name`, the
                // name is the argument which would otherwise be the pattern.
                opts->master_setup_given = true;
                opts->master_setup = optarg;
                break;
            case PASSGEN_CLI_OPT_MASTER_LEVEL:
                if(optarg[0] < '1' ||
                   optarg[0] > '0' + PASSGEN_MASTER_LEVEL_MAX || optarg[1]) {
                    return passgen_cli_bail(
                        PASSGEN_CLI_ERROR_MASTER_CONFLICT,
                        "--master-level has to be from 1 to 3");
                }
                opts->master_setup_level = (unsigned) (optarg[0] - '0');
                break;
            case PASSGEN_CLI_OPT_MASTER_SEED:
                opts->master_setup_seed = true;
                break;
            case PASSGEN_CLI_OPT_MASTER_VERSION:
                // there is only one, but saying which one is used means that
                // a script keeps setting up the same, also once there are
                // more.
                if(0 != strcmp(optarg, "1")) {
                    return passgen_cli_bail(
                        PASSGEN_CLI_ERROR_MASTER_CONFLICT,
                        "--master-version has to be 1");
                }
                opts->master_setup_version = true;
                break;
#endif
            case 'c':
                opts->configs[opts->configs_len++] = optarg;
                break;
            case 'p':
                opts->preset = optarg;
                break;
            case PASSGEN_CLI_OPT_OFFSET: {
                char *end;
                errno = 0;
                unsigned long long offset = strtoull(optarg, &end, 10);
                if(*optarg < '0' || *optarg > '9' || *end || errno ||
                   offset > UINT64_MAX) {
                    fprintf(
                        stderr,
                        "\033[1;31merror\033[0m: illegal offset entered "
                        "(%s)\n",
                        optarg);
                    return PASSGEN_CLI_ERROR_ILLEGAL_OFFSET;
                }
                opts->offset = (uint64_t) offset;
                opts->offset_given = true;
                break;
            }
            case 'S': {
                char *end;
                errno = 0;
                unsigned long long steps = strtoull(optarg, &end, 10);
                if(*optarg < '0' || *optarg > '9' || *end || errno ||
                   steps < 1 || steps > SIZE_MAX) {
                    return passgen_cli_bail(
                        PASSGEN_CLI_ERROR_ILLEGAL_STEPS,
                        optarg);
                }
                passgen_env_step_limit_set(opts->env, (size_t) steps);
                break;
            }
            case 'z':
                opts->null = true;
                break;
            case 'e':
                opts->entropy = true;
                break;
            case 'v':
                return PASSGEN_CLI_SHOW_VERSION;
            case 'l':
                opts->list = true;
                break;
            case 'w':
            case 'P':
                // added once the configuration is loaded.
                opts->definitions[opts->definitions_len++] =
                    (passgen_cli_definition){opt, optarg};
                break;
            case 'r':
                try(passgen_cli_opts_random(opts, optarg));
                random_given = true;
                break;
            case 'j':
                opts->json = true;
                break;
            case '?':
            default:
                return passgen_cli_bail(
                    PASSGEN_CLI_ERROR_UNRECOGNIZED,
                    argv[optind]);
        }
    }

    // the stream of the last sequence has to exist.
    if(opts->amount - 1 > UINT64_MAX - opts->offset) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: the offset is too large for the amount\n");
        return PASSGEN_CLI_ERROR_ILLEGAL_OFFSET;
    }

    // a pattern, or a preset to take it from (which is looked up once the
    // configuration is loaded), but not both.
#ifdef PASSGEN_MONOCYPHER
    if(opts->master_setup_given && (argc - optind) > 1) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--master-setup takes one name, and no pattern");
    }
#endif
    if(optind < argc) {
        if(opts->preset || (argc - optind) > 1) {
            return passgen_cli_bail(
                PASSGEN_CLI_ERROR_MULTIPLE_FORMATS,
                argv[optind]);
        } else {
            opts->pattern = argv[optind];
        }
    }

    // when setting up a master passphrase, there is no pattern, but there may
    // be its name.
    bool setup = false;
#ifdef PASSGEN_MONOCYPHER
    setup = opts->master_setup_given;
    if(setup && opts->pattern) {
        if(opts->master_setup) {
            return passgen_cli_bail(
                PASSGEN_CLI_ERROR_MASTER_CONFLICT,
                "--master-setup cannot be used with a pattern");
        }
        opts->master_setup = opts->pattern;
        opts->pattern = NULL;
    }
#endif

    // a login for a domain can say which pattern to use.
    bool login = false;
#ifdef PASSGEN_MONOCYPHER
    login = opts->master_domain != NULL;
#endif

    // if no pattern or preset was given, show help, unless the presets are
    // to be listed, or a master passphrase is set up.
    if(!opts->pattern && !opts->preset && !opts->list && !setup && !login) {
        return PASSGEN_CLI_SHOW_HELP;
    }

#ifdef PASSGEN_MONOCYPHER
    // master passphrase mode is a source of randomness, so it cannot be
    // combined with another one. silently preferring either of them would
    // result in something other than what was asked for.
    if(opts->master && random_given) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--master and --random cannot be used together");
    }

    // a domain without a master passphrase is for the login with it, which
    // says which one, so it cannot be used with another source of
    // randomness either. a user without either does nothing on its own.
    if(opts->master_domain && random_given) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--domain is for master passphrase mode, which cannot be used "
            "with --random");
    }
    if(!opts->master && !opts->master_domain && opts->master_user) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--user needs --domain or --master");
    }

    // setting one up is something else than generating.
    if(setup && (opts->master || opts->pattern || opts->preset || opts->list ||
                 opts->master_domain || opts->master_user)) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--master-setup cannot be used with a pattern, --preset, --list, "
            "--master, --domain or --user");
    }
    if(!setup && (opts->master_setup_level || opts->master_setup_seed ||
                  opts->master_setup_version)) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--master-level, --master-seed and --master-version need "
            "--master-setup");
    }

    // the source of randomness is opened once the master passphrase is
    // entered, which is after the pattern is checked. which master passphrase
    // a login uses is known once the configuration is loaded.
    if(opts->master || opts->master_domain) {
        return 0;
    }
#else
    // only master passphrase mode needs to know about this.
    (void) random_given;
#endif

    // fallback to system randomness if nothing else is defined
    if(!opts->random) {
        opts->random = passgen_random_open(NULL, NULL);
        if(!opts->random) {
            return passgen_cli_bail(PASSGEN_CLI_ERROR_RANDOM_ALLOC, NULL);
        }
    }

    // this is what the environment generates from.
    passgen_env_random_set(opts->env, opts->random);

    return 0;
}

// the help for master passphrase mode, and for the sources of randomness
// which it is built on, which are only there when building with monocypher.
// clang-format off
#ifdef PASSGEN_MONOCYPHER
#define PASSGEN_CLI_HELP_DOMAIN \
    "    -d, --domain DOMAIN    Domain for master passphrase mode, which uses\n" \
    "                           its login in the configuration without -m\n"
#define PASSGEN_CLI_HELP_MASTER                                               \
    "    -m, --master NAME      Derive the randomness from a master passphrase,\n" \
    "                           which is asked for\n"                          \
    "        --master-setup [NAME]\n"                                          \
    "                           Set up a master passphrase for the\n"         \
    "                           configuration, asking how\n"
#define PASSGEN_CLI_HELP_USER \
    "    -u, --user USER        User (login) for master passphrase mode\n"
#define PASSGEN_CLI_HELP_CHACHA20                                             \
    "    chacha20:KEY[:IV]      ChaCha20, key and IV in hex (64 and 16 digits)\n" \
    "    master:VERSION:LEVEL:PASS:SEED:DOMAIN:USER\n"                         \
    "                           What --master uses, with \\: and \\\\ for : and \\\n"
#else
#define PASSGEN_CLI_HELP_DOMAIN   ""
#define PASSGEN_CLI_HELP_MASTER   ""
#define PASSGEN_CLI_HELP_USER     ""
#define PASSGEN_CLI_HELP_CHACHA20 ""
#endif
// clang-format on

void passgen_cli_usage(const char *executable) {
    passgen_version version = passgen_version_get();
    fprintf(
        stderr,
        "passgen version v%hu.%hu.%hu\n",
        version.major,
        version.minor,
        version.patch);
    // one option per line, which is easier to keep up to date than what
    // clang-format makes of it.
    // clang-format off
    fprintf(
        stderr,
        "Generate random sequences from a regex-like pattern.\n"
        "\n"
        "Usage: %s [OPTIONS] PATTERN\n"
        "       %s [OPTIONS] -p PRESET\n"
        "\n"
        "OPTIONS\n"
        "    -a, --amount N         Generate N sequences (default: 1)\n"
        "    -c, --config PATH      Read this configuration, not the usual ones\n"
        PASSGEN_CLI_HELP_DOMAIN
        "    -e, --entropy          Show the entropy of every sequence, in bits\n"
        "    -h, --help             Show this help\n"
        "    -j, --json             Output as JSON\n"
        "    -l, --list             List the presets\n"
        PASSGEN_CLI_HELP_MASTER
        "        --offset N         Start at the Nth of the sequences of a\n"
        "                           deterministic source of randomness\n"
        "                           (default: 0)\n"
        "    -p, --preset NAME      Use the pattern of a preset\n"
        "    -P, --define-preset NAME:PATTERN\n"
        "                           Define a preset\n"
        "    -r, --random SOURCE    Source of randomness (default: system)\n"
        "    -S, --steps N          Limit on steps to generate (default: 1000000)\n"
        PASSGEN_CLI_HELP_USER
        "    -v, --version          Show the version\n"
        "    -w, --wordlist NAME:PATH\n"
        "                           Load the wordlist NAME from PATH\n"
        "    -z, --null             Separate outputs with NUL instead of newlines\n"
        "\n"
        "SOURCES OF RANDOMNESS\n"
        "    system                 The secure one of the system (default)\n"
        "    file:PATH              Read from a file\n"
        PASSGEN_CLI_HELP_CHACHA20
        "    xorshift:SEED          Xorshift, which is not secure\n"
        "    zero                   Only zeroes, for testing\n"
        "\n"
        "PATTERN\n"
        "    abc                    Literal: outputs 'abc'\n"
        "    [a-z0-9:]              Set: a lowercase letter, a digit or ':'\n"
        "    (abc|def)              Group: 'abc' or 'def'\n"
        "    ({3}abc|def)           Weights: 'abc' is three times as likely\n"
        "    a{3}, a{2,4}           Repetition: 'aaa', or two to four times 'a'\n"
        "    a?                     Optional: 'a' or nothing\n"
        "    \\w{name}               Word: a word from the wordlist\n"
        "    \\m{name}               Markov: a pronounceable word from the wordlist\n"
        "    \\m{name:2}             The same, with a chain of length 2 (0 to 8,\n"
        "                           default: 3)\n"
        "    \\p{name}               Preset: the pattern of the preset\n"
        "    \\u{e4}                 A character, by its codepoint in hex\n"
        "\n"
        "See passgen(1) for more.\n",
        executable,
        executable);
    // clang-format on
}

void passgen_cli_show_version(void) {
    passgen_version version = passgen_version_get();
    fprintf(
        stderr,
        "passgen version v%hu.%hu.%hu",
        version.major,
        version.minor,
        version.patch);

    // not every build knows what it was built from.
    if(version.build && version.build[0]) {
        fprintf(stderr, " (build %s)", version.build);
    }
    fprintf(stderr, "\n");

    if(passgen_is_debug()) {
        fprintf(stderr, "debug build\n");
    } else {
        fprintf(stderr, "release build\n");
    }
}

int passgen_cli_bail(passgen_cli_error error, const void *data) {
    switch(error) {
        case PASSGEN_CLI_ERROR_MULTIPLE_FORMATS:
            fprintf(
                stderr,
                "error: multiple patterns specified (%s).\n",
                (const char *) data);
            break;
        case PASSGEN_CLI_ERROR_RANDOM_ALLOC:
            fprintf(stderr, "error: cannot open the source of randomness.\n");
            break;
        case PASSGEN_CLI_ERROR_PATTERN_PARSE:
            break;
        case PASSGEN_CLI_ERROR_ILLEGAL_AMOUNT:
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: illegal amount entered (%i)\n",
                *((const int *) data));
            break;
        case PASSGEN_CLI_ERROR_UNRECOGNIZED:
            break;
        case PASSGEN_CLI_ERROR_MASTER_CONFLICT:
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: %s\n",
                (const char *) data);
            break;
        case PASSGEN_CLI_ERROR_UNUSED_11:
            break;
        case PASSGEN_CLI_ERROR_ILLEGAL_STEPS:
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: illegal number of steps entered "
                "(%s)\n",
                (const char *) data);
            break;
        case PASSGEN_CLI_ERROR_GENERATE:
            // what went wrong was printed by whoever noticed.
            break;
        case PASSGEN_CLI_ERROR_ALLOC:
            fprintf(stderr, "\033[1;31merror\033[0m: out of memory\n");
            break;
        case PASSGEN_CLI_ERROR_WORDLIST:
        case PASSGEN_CLI_ERROR_CONFIG:
        case PASSGEN_CLI_ERROR_MASTER:
        case PASSGEN_CLI_ERROR_ILLEGAL_OFFSET:
            // what went wrong was printed by whoever noticed.
            break;
        case PASSGEN_CLI_ERROR_PRESET_NOT_FOUND:
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: preset `%s` not found\n",
                (const char *) data);
            break;
        default:
            fprintf(stderr, "error: unknown error.\n");
            break;
    }

    return error;
}

void passgen_cli_opts_free(passgen_cli_opts *opts) {
    passgen_env_free(opts->env);
    free(opts->definitions);
    free(opts->configs);
    for(size_t i = 0; i < opts->masters_len; i++) {
        passgen_cli_master_free(&opts->masters[i]);
    }
    free(opts->masters);
    for(size_t i = 0; i < opts->logins_len; i++) {
        passgen_cli_login_free(&opts->logins[i]);
    }
    free(opts->logins);

    // the environment only borrows the source of randomness.
    if(opts->random) {
        passgen_random_free(opts->random);
    }
}

int passgen_cli_opts_load(passgen_cli_opts *opts) {
    try(passgen_cli_opts_config(opts));

    // what is defined on the command line replaces what the configuration
    // defines.
    for(size_t i = 0; i < opts->definitions_len; i++) {
        const passgen_cli_definition *definition = &opts->definitions[i];
        if(definition->option == 'P') {
            try(passgen_cli_opts_preset(opts, definition->argument));
        } else {
            try(passgen_cli_opts_wordlist(opts, definition->argument));
        }
    }

#ifdef PASSGEN_MONOCYPHER
    // a login fills in what was not given.
    if(opts->master_domain) {
        try(passgen_cli_login_apply(opts));
        if(!opts->pattern && !opts->preset) {
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: there is no login for `%s` which says "
                "which pattern to use, give one\n",
                opts->master_domain);
            return PASSGEN_CLI_ERROR_MASTER;
        }
    }
#endif

    // listing the presets does not need a pattern.
    if(opts->preset && !opts->list) {
        opts->pattern = passgen_env_preset_get(opts->env, opts->preset);
        if(!opts->pattern) {
            return passgen_cli_bail(
                PASSGEN_CLI_ERROR_PRESET_NOT_FOUND,
                opts->preset);
        }
    }

    return 0;
}

#include "passgen.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
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


#define UNUSED(x)              (void) x
#define strprefix(prefix, str) strncmp(prefix, str, strlen(prefix))

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

/// Prints a pattern, and marks a part of it.
static void passgen_cli_pattern_mark(
    const char *prefix,
    const char *pattern,
    const passgen_error *error) {
    fprintf(stderr, "\033[1;33m%s\033[0m%s\n", prefix, pattern);

    // this lines up as long as every character is as wide as the others,
    // which is not the case for all of them. it is good enough to find the
    // place.
    for(size_t i = 0; i < strlen(prefix) + error->start.codepoint; i++) {
        fputc(' ', stderr);
    }

    // errors at the end of the pattern, where something is missing, don't
    // have a length.
    size_t length = error->end.codepoint - error->start.codepoint;
    fprintf(stderr, "\033[1;31m");
    for(size_t i = 0; i < (length ? length : 1); i++) {
        fputc('^', stderr);
    }
    fprintf(stderr, "\033[0m\n");
}

/// Prints what is wrong with a pattern, and marks where in the pattern it is.
/// For problems in the presets which it uses, it marks where in their
/// patterns they are, too.
static void passgen_cli_pattern_error(
    const passgen_error *error,
    const char *pattern,
    const passgen_env *env) {
    fprintf(stderr, "\033[1;31merror\033[0m in pattern: ");
    passgen_error_print(error, stderr);

    if(error->has_position) {
        passgen_cli_pattern_mark("pattern: ", pattern, error);
    }

    // what caused a problem with a preset is in its pattern, which is found by
    // its name.
    for(const passgen_error *current = error; current->cause;
        current = current->cause) {
        if(current->kind != PASSGEN_ERROR_PRESET_INVALID ||
           !current->cause->has_position) {
            continue;
        }

        const char *preset = passgen_env_preset_get(env, current->detail);
        if(!preset) {
            continue;
        }

        char prefix[sizeof(current->detail) + 16];
        snprintf(prefix, sizeof(prefix), "preset %s: ", current->detail);
        passgen_cli_pattern_mark(prefix, preset, current->cause);
    }
}

int passgen_cli_run(passgen_cli_opts opts) {
    passgen_pattern *pattern;
    passgen_error error;
    if(PASSGEN_OK != passgen_parse(&pattern, &error, opts.pattern)) {
        passgen_cli_pattern_error(&error, opts.pattern, opts.env);
        passgen_error_free(&error);
        return PASSGEN_CLI_ERROR_PATTERN_PARSE;
    }

    // this reads the wordlists which the pattern uses, so problems with them
    // come up before anything is generated, and point to where they are used.
    if(PASSGEN_OK != passgen_resolve(pattern, opts.env, &error)) {
        passgen_cli_pattern_error(&error, opts.pattern, opts.env);
        passgen_error_free(&error);
        passgen_pattern_free(pattern);
        return PASSGEN_CLI_ERROR_GENERATE;
    }

    int ret = passgen_cli_generate(opts, pattern);
    passgen_pattern_free(pattern);
    return ret;
}

/// Growable output buffer. Generated sequences have no fixed upper bound on
/// their length, so they are collected in here.
typedef struct {
    char *data;
    size_t len;
    size_t capacity;
    bool json;
} passgen_cli_buffer;

static int
passgen_cli_buffer_push(passgen_cli_buffer *buffer, const char *data, size_t len) {
    if(buffer->capacity - buffer->len < len) {
        size_t capacity = buffer->capacity ? buffer->capacity : 256;
        while(capacity - buffer->len < len) {
            capacity *= 2;
        }
        char *new_data = realloc(buffer->data, capacity);
        if(!new_data) {
            return -1;
        }
        buffer->data = new_data;
        buffer->capacity = capacity;
    }

    memcpy(&buffer->data[buffer->len], data, len);
    buffer->len += len;
    return 0;
}

/// Encodes a codepoint as UTF-8, and returns how many bytes that took, or
/// zero if it is not a codepoint which can be encoded.
static int passgen_cli_utf8_encode(uint8_t *out, uint32_t codepoint) {
    if(codepoint <= 0x7F) {
        out[0] = (uint8_t) codepoint;
        return 1;
    }
    if(codepoint <= 0x7FF) {
        out[0] = (uint8_t) (0xC0 | (codepoint >> 6));
        out[1] = (uint8_t) (0x80 | (codepoint & 0x3F));
        return 2;
    }
    if(codepoint <= 0xFFFF) {
        out[0] = (uint8_t) (0xE0 | (codepoint >> 12));
        out[1] = (uint8_t) (0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = (uint8_t) (0x80 | (codepoint & 0x3F));
        return 3;
    }
    if(codepoint <= 0x10FFFF) {
        out[0] = (uint8_t) (0xF0 | (codepoint >> 18));
        out[1] = (uint8_t) (0x80 | ((codepoint >> 12) & 0x3F));
        out[2] = (uint8_t) (0x80 | ((codepoint >> 6) & 0x3F));
        out[3] = (uint8_t) (0x80 | (codepoint & 0x3F));
        return 4;
    }
    return 0;
}

static int passgen_cli_buffer_puts(passgen_cli_buffer *buffer, const char *str) {
    return passgen_cli_buffer_push(buffer, str, strlen(str));
}

/// Callback for passgen_generate(): appends a codepoint as UTF-8 to the
/// buffer, escaping it for use in a JSON string if requested.
static int passgen_cli_buffer_emit(void *data, uint32_t codepoint) {
    passgen_cli_buffer *buffer = data;
    char encoded[8];

    if(buffer->json) {
        switch(codepoint) {
            case '"':
                return passgen_cli_buffer_puts(buffer, "\\\"");
            case '\\':
                return passgen_cli_buffer_puts(buffer, "\\\\");
            case '\b':
                return passgen_cli_buffer_puts(buffer, "\\b");
            case '\f':
                return passgen_cli_buffer_puts(buffer, "\\f");
            case '\n':
                return passgen_cli_buffer_puts(buffer, "\\n");
            case '\r':
                return passgen_cli_buffer_puts(buffer, "\\r");
            case '\t':
                return passgen_cli_buffer_puts(buffer, "\\t");
            default:
                break;
        }

        if(codepoint < 0x20) {
            snprintf(encoded, sizeof(encoded), "\\u%04x", (unsigned) codepoint);
            return passgen_cli_buffer_puts(buffer, encoded);
        }
    }

    int bytes = passgen_cli_utf8_encode((uint8_t *) encoded, codepoint);
    if(bytes <= 0) {
        return -1;
    }

    return passgen_cli_buffer_push(buffer, encoded, bytes);
}

/// Reports why generating failed, and returns what to exit with.
static int
passgen_cli_generate_error(passgen_status status, passgen_error *error) {
    // the only reason for the callback to stop generating is that the buffer
    // for the output could not be made larger.
    if(status == PASSGEN_ERROR_ABORTED) {
        passgen_error_free(error);
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }

    fprintf(stderr, "\033[1;31merror\033[0m: ");
    passgen_error_print(error, stderr);
    passgen_error_free(error);

    return PASSGEN_CLI_ERROR_GENERATE;
}

int passgen_cli_generate_normal(
    passgen_cli_opts opts,
    passgen_pattern *pattern) {
    passgen_cli_buffer buffer = {0};
    char sep = opts.null ? '\0' : '\n';

    for(size_t i = 0; i < opts.amount; ++i) {
        double entropy;
        passgen_error error;
        buffer.len = 0;
        passgen_status status = passgen_generate(
            pattern,
            opts.env,
            &error,
            opts.entropy ? &entropy : NULL,
            &buffer,
            passgen_cli_buffer_emit);
        if(status != PASSGEN_OK) {
            free(buffer.data);
            return passgen_cli_generate_error(status, &error);
        }

        if(opts.entropy) {
            fprintf(stderr, "entropy: %.2lf bits\n", entropy);
        }

        // the separator (or the newline after the last one) follows right
        // away, and it is written out before the entropy of the next one goes
        // to standard error, which is not buffered. otherwise they would come
        // out of order where both go to the same place, such as a terminal.
        fwrite(buffer.data, 1, buffer.len, stdout);
        fputc(i + 1 < opts.amount ? sep : '\n', stdout);
        fflush(stdout);
    }

    free(buffer.data);

    return 0;
}

int passgen_cli_generate_json(passgen_cli_opts opts, passgen_pattern *pattern) {
    // the whole document is collected first, so that nothing is printed (rather
    // than truncated JSON) when generating fails.
    passgen_cli_buffer buffer = {.json = true};

    // this is nonzero when the buffer could not be made larger.
    int ret = passgen_cli_buffer_puts(&buffer, "[");

    for(size_t i = 0; ret == 0 && i < opts.amount; ++i) {
        double entropy;
        ret = passgen_cli_buffer_puts(
            &buffer,
            (i == 0) ? "{\"output\":\"" : ",{\"output\":\"");
        if(ret != 0) {
            break;
        }

        passgen_error error;
        passgen_status status = passgen_generate(
            pattern,
            opts.env,
            &error,
            opts.entropy ? &entropy : NULL,
            &buffer,
            passgen_cli_buffer_emit);
        if(status != PASSGEN_OK) {
            free(buffer.data);
            return passgen_cli_generate_error(status, &error);
        }

        if(opts.entropy) {
            char number[64];
            snprintf(
                number,
                sizeof(number),
                "\",\"entropy\":%lf}",
                entropy);
            ret = passgen_cli_buffer_puts(&buffer, number);
        } else {
            ret = passgen_cli_buffer_puts(&buffer, "\"}");
        }
    }

    if(ret == 0) {
        ret = passgen_cli_buffer_puts(&buffer, "]\n");
    }

    if(ret != 0) {
        free(buffer.data);
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }

    fwrite(buffer.data, 1, buffer.len, stdout);
    free(buffer.data);
    return 0;
}

int passgen_cli_generate(passgen_cli_opts opts, passgen_pattern *pattern) {
    if(!opts.json) {
        return passgen_cli_generate_normal(opts, pattern);
    } else {
        return passgen_cli_generate_json(opts, pattern);
    }
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

    FILE *file = fopen(colon + 1, "r");
    if(!file) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: cannot open wordlist `%s`\n",
            colon + 1);
        return PASSGEN_CLI_ERROR_WORDLIST;
    }

    char *name = malloc(offset + 1);
    if(!name) {
        fclose(file);
        return PASSGEN_CLI_ERROR_ALLOC;
    }
    memcpy(name, input, offset);
    name[offset] = 0;

    // the name is copied, and the file is closed, whether this works or not.
    passgen_status status =
        passgen_env_wordlist_add(opt->env, name, file);
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
    opts->amount = 1;
    opts->null = false;
    opts->json = false;
    opts->entropy = false;
#ifdef PASSGEN_MONOCYPHER
    opts->master_passphrase = NULL;
    opts->master_domain = NULL;
    opts->master_token = NULL;
#endif
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

/// A preset, for listing them.
typedef struct {
    const char *name;
    const char *pattern;
} passgen_cli_listed_preset;

/// The presets which are listed, which grows as they are collected.
typedef struct {
    passgen_cli_listed_preset *items;
    size_t count;
    size_t capacity;
} passgen_cli_listed_presets;

static int passgen_cli_presets_collect(
    void *user,
    const char *name,
    const char *pattern) {
    passgen_cli_listed_presets *list = user;

    // presets whose names start with an underscore are parts of other
    // presets, which are not meant to be used on their own, so they are not
    // listed. (an underscore rather than a dot, which TOML keys cannot start
    // with.)
    if(name[0] == '_') {
        return 0;
    }

    if(list->count == list->capacity) {
        size_t capacity = list->capacity ? list->capacity * 2 : 16;
        passgen_cli_listed_preset *items =
            realloc(list->items, capacity * sizeof(*items));
        if(!items) {
            return -1;
        }
        list->items = items;
        list->capacity = capacity;
    }

    list->items[list->count].name = name;
    list->items[list->count].pattern = pattern;
    list->count += 1;
    return 0;
}

static int passgen_cli_presets_compare(const void *lhs, const void *rhs) {
    const passgen_cli_listed_preset *a = lhs;
    const passgen_cli_listed_preset *b = rhs;
    return strcmp(a->name, b->name);
}

static int passgen_cli_presets_list(const passgen_cli_opts *opts) {
    passgen_cli_listed_presets list = {0};
    if(0 != passgen_env_preset_foreach(
                opts->env,
                &list,
                passgen_cli_presets_collect)) {
        free(list.items);
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }

    // they are kept in no particular order, but listed by name.
    if(list.count) {
        qsort(
            list.items,
            list.count,
            sizeof(list.items[0]),
            passgen_cli_presets_compare);
    }

    for(size_t i = 0; i < list.count; i++) {
        printf("preset %s %s\n", list.items[i].name, list.items[i].pattern);
    }

    free(list.items);
    return 0;
}

// the options for master passphrase mode only exist when it does, which is
// when building with monocypher.
#ifdef PASSGEN_MONOCYPHER
#define PASSGEN_CLI_MASTER_OPTS "d:m:t:"
#else
#define PASSGEN_CLI_MASTER_OPTS ""
#endif

int passgen_cli_opts_parse(passgen_cli_opts *opts, int argc, char *argv[]) {
    const char *short_opts = "a:p:P:w:r:S:ezhvjl" PASSGEN_CLI_MASTER_OPTS;
    const char *preset = NULL;
    // whether the randomness source was set on the command line, rather than
    // in the configuration.
    bool random_given = false;

    static struct option long_opts[] = {
        {"amount", required_argument, NULL, 'a'},
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
        {"token", required_argument, NULL, 't'},
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
                    return passgen_cli_bail(PASSGEN_CLI_ERROR_ILLEGAL_AMOUNT, &opt);
                }
                opts->amount = opt;
                break;
#ifdef PASSGEN_MONOCYPHER
            case 'm':
                opts->master_passphrase = optarg;
                break;
            case 'd':
                opts->master_domain = optarg;
                break;
            case 't':
                opts->master_token = optarg;
                break;
#endif
            case 'p':
                preset = optarg;
                break;
            case 'S': {
                char *end;
                errno = 0;
                unsigned long long steps = strtoull(optarg, &end, 10);
                if(*optarg < '0' || *optarg > '9' || *end || errno ||
                   steps < 1 || steps > SIZE_MAX) {
                    return passgen_cli_bail(PASSGEN_CLI_ERROR_ILLEGAL_STEPS, optarg);
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
                exit(passgen_cli_presets_list(opts));
            case 'w':
                try(passgen_cli_opts_wordlist(opts, optarg));
                break;
            case 'r':
                try(passgen_cli_opts_random(opts, optarg));
                random_given = true;
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
                    PASSGEN_CLI_ERROR_UNRECOGNIZED,
                    argv[optind]);
        }
    }

    // can't have both a preset and a format at the same time.
    if(preset && opts->pattern) {
        return passgen_cli_bail(PASSGEN_CLI_ERROR_MULTIPLE_FORMATS, opts->pattern);
    }

    // if a preset was given, parse it.
    if(preset) {
        opts->pattern = passgen_env_preset_get(opts->env, preset);
        if(!opts->pattern) {
            return passgen_cli_bail(PASSGEN_CLI_ERROR_PRESET_NOT_FOUND, preset);
        }
    }

    // parse a given format, making sure we don't have multiple."
    if(optind < argc) {
        if(opts->pattern || (argc - optind) > 1) {
            return passgen_cli_bail(
                PASSGEN_CLI_ERROR_MULTIPLE_FORMATS,
                argv[optind]);
        } else {
            opts->pattern = argv[optind];
        }
    }

    // if no format or preset was given, show help.
    if(!opts->pattern) {
        return PASSGEN_CLI_SHOW_HELP;
    }

#ifdef PASSGEN_MONOCYPHER
    // master passphrase mode is a source of randomness, so it cannot be
    // combined with another one. silently preferring either of them would
    // result in something other than what was asked for.
    if(opts->master_passphrase && random_given) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--master and --random cannot be used together");
    }

    // these don't do anything on their own. most likely, the master
    // passphrase was forgotten.
    if(!opts->master_passphrase && (opts->master_domain || opts->master_token)) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--domain and --token need --master");
    }

    if(opts->master_passphrase) {
        // this takes precedence over a randomness source which was set in
        // the configuration.
        if(opts->random) {
            passgen_random_free(opts->random);
        }

        opts->random = passgen_random_chacha20_argon2_open(
            NULL,
            opts->master_passphrase,
            opts->master_domain,
            opts->master_token,
            NULL);
        if(!opts->random) {
            return passgen_cli_bail(PASSGEN_CLI_ERROR_RANDOM_ALLOC, NULL);
        }
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
#ifdef PASSGEN_MONOCYPHER
#define PASSGEN_CLI_HELP_DOMAIN \
    "    -d, --domain DOMAIN    Domain for master passphrase mode\n"
#define PASSGEN_CLI_HELP_MASTER \
    "    -m, --master PASS      Derive the randomness from a master passphrase\n"
#define PASSGEN_CLI_HELP_TOKEN \
    "    -t, --token TOKEN      Token for master passphrase mode\n"
#define PASSGEN_CLI_HELP_CHACHA20                                             \
    "    chacha20:KEY[:IV]      ChaCha20, key and IV in hex (64 and 16 digits)\n" \
    "    chacha20-argon2:PASS[:DOMAIN[:TOKEN]]\n"                              \
    "                           What --master uses\n"
#else
#define PASSGEN_CLI_HELP_DOMAIN   ""
#define PASSGEN_CLI_HELP_MASTER   ""
#define PASSGEN_CLI_HELP_TOKEN    ""
#define PASSGEN_CLI_HELP_CHACHA20 ""
#endif

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
        "\n"
        "Usage: %s [OPTIONS] PATTERN\n"
        "       %s [OPTIONS] -p PRESET\n"
        "\n"
        "OPTIONS\n"
        "    -a, --amount N         Generate N sequences (default: 1)\n"
        PASSGEN_CLI_HELP_DOMAIN
        "    -e, --entropy          Show the entropy of every sequence, in bits\n"
        "    -h, --help             Show this help\n"
        "    -j, --json             Output as JSON\n"
        "    -l, --list             List the presets\n"
        PASSGEN_CLI_HELP_MASTER
        "    -p, --preset NAME      Use the pattern of a preset\n"
        "    -P, --define-preset NAME:PATTERN\n"
        "                           Define a preset\n"
        "    -r, --random SOURCE    Source of randomness (default: system)\n"
        "    -S, --steps N          Limit on steps to generate (default: 1000000)\n"
        PASSGEN_CLI_HELP_TOKEN
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
        "    \\m{name:2}             The same, with a chain of length 2 (1 to 8,\n"
        "                           default: 3)\n"
        "    \\p{name}               Preset: the pattern of the preset\n"
        "    \\u{e4}                 A character, by its codepoint in hex\n"
        "\n"
        "See passgen(1) for more.\n",
        executable,
        executable);
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
                "\033[1;31merror\033[0m: illegal number of steps entered (%s)\n",
                (const char *) data);
            break;
        case PASSGEN_CLI_ERROR_GENERATE:
            // what went wrong was printed by whoever noticed.
            break;
        case PASSGEN_CLI_ERROR_ALLOC:
            fprintf(stderr, "\033[1;31merror\033[0m: out of memory\n");
            break;
        case PASSGEN_CLI_ERROR_WORDLIST:
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

    // the environment only borrows the source of randomness.
    if(opts->random) {
        passgen_random_free(opts->random);
    }
}

int passgen_cli_opts_config_load(passgen_cli_opts *opts, FILE *file) {
    // read the whole file, however large it is.
    size_t capacity = 4096;
    size_t length = 0;
    char *config = malloc(capacity);
    if(!config) {
        return PASSGEN_CLI_ERROR_ALLOC;
    }

    while(true) {
        // always leaves room for the NUL terminator
        length += fread(&config[length], 1, capacity - length - 1, file);
        if(length < capacity - 1) {
            break;
        }

        capacity *= 2;
        char *new_config = realloc(config, capacity);
        if(!new_config) {
            free(config);
            return PASSGEN_CLI_ERROR_ALLOC;
        }
        config = new_config;
    }

    config[length] = 0;
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
    // $XDG_CONFIG_HOME is what the XDG base directory specification defines,
    // $XDG_CONFIG_PATH is what earlier versions of passgen looked at.
    const char *dir = getenv("XDG_CONFIG_HOME");
    if(!dir || !*dir) {
        dir = getenv("XDG_CONFIG_PATH");
    }

    const char *suffix = "/passgen.conf";
    if(!dir || !*dir) {
        dir = getenv("HOME");
        suffix = "/.config/passgen.conf";
        if(!dir) {
            fprintf(stderr, "warning: cannot locate home directory\n");
            return 0;
        }
    }

    size_t config_path_len = strlen(dir) + strlen(suffix) + 1;
    char *config_path = malloc(config_path_len);
    if(!config_path) {
        return PASSGEN_CLI_ERROR_ALLOC;
    }
    snprintf(config_path, config_path_len, "%s%s", dir, suffix);
    FILE *file = fopen(config_path, "r");
    free(config_path);
    if(file) {
        int ret = passgen_cli_opts_config_load(opts, file);
        fclose(file);
        return ret;
    } else {
        return 0;
    }
}

#ifdef PASSGEN_SECCOMP
#include <sys/ioctl.h>
// TCGETS2 is defined with the size of struct termios2, which only the kernel
// headers declare. this is fine as long as <termios.h> is not included, which
// declares its own struct termios.
#ifdef __linux__
#include <asm/termbits.h>
#endif

// musl uses this, glibc doesn't have it by default.
#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

void passgen_cli_seccomp_init(void) {
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
    // musl sets O_LARGEFILE. architectures which only have openat (such as
    // aarch64 and riscv64) need it there as well.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(open),
        1,
        SCMP_CMP(1, SCMP_CMP_EQ, O_RDONLY | O_LARGEFILE));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(openat),
        1,
        SCMP_CMP(2, SCMP_CMP_EQ, O_RDONLY | O_LARGEFILE));

    // allow reading stat of files
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(newfstatat), 0);

    // allow reading and seeking in files
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(readv), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);

    // allow writing, but only to stdout or stderr.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(write),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 1));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(write),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 2));
    // musl uses writev
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(writev),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 1));
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(writev),
        1,
        SCMP_CMP(0, SCMP_CMP_EQ, 2));

    // allow closing, as long as the fd is not any of the standard streams.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(close),
        1,
        SCMP_CMP(0, SCMP_CMP_GT, 2));

    // allow reading random data
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getrandom), 0);

    // newer versions of glibc (2.41 and later) implement getrandom() in the
    // vDSO, and the first call blocks signals while it sets up its state.
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask), 0);

    // allow exiting
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);

    // allow allocating memory
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);

    // allow mapping anonymous memory (used by allocator)
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(mmap),
        3,
        SCMP_CMP(0, SCMP_CMP_EQ, 0),
        SCMP_CMP(2, SCMP_CMP_EQ, PROT_READ | PROT_WRITE),
        SCMP_CMP(3, SCMP_CMP_EQ, MAP_PRIVATE | MAP_ANONYMOUS));

    // the state of getrandom() in the vDSO is in memory which the kernel may
    // drop (MAP_DROPPABLE, Linux 6.11 and later). headers which are older
    // than that do not define it, but glibc may still use it.
#ifndef MAP_DROPPABLE
#define MAP_DROPPABLE 0x08
#endif
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(mmap),
        3,
        SCMP_CMP(0, SCMP_CMP_EQ, 0),
        SCMP_CMP(2, SCMP_CMP_EQ, PROT_READ | PROT_WRITE),
        SCMP_CMP(3, SCMP_CMP_EQ, MAP_DROPPABLE | MAP_ANONYMOUS));

    // allow unmapping memory (used by allocation)
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);

    // allow remapping memory (used by allocation)
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mremap), 0);

    // allow advice about memory, which is only about the memory of this
    // process. the allocator of glibc uses it for large allocations (such as
    // the memory for deriving the key from a master passphrase, if
    // transparent huge pages are enabled), and when freeing memory.
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(madvise), 0);

    // allow checking for terminal window size
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(ioctl),
        2,
        SCMP_CMP(0, SCMP_CMP_EQ, 1),
        SCMP_CMP(1, SCMP_CMP_EQ, TIOCGWINSZ));

    // allow checking whether stdout or stderr are a terminal (which only
    // reads its settings). the C library does this before writing to them
    // when they are a character device, such as /dev/null, to decide how to
    // buffer: without this, `passgen a > /dev/null` was killed. newer
    // versions of glibc use TCGETS2 on some architectures.
    for(int fd = 1; fd <= 2; fd++) {
        seccomp_rule_add(
            ctx,
            SCMP_ACT_ALLOW,
            SCMP_SYS(ioctl),
            2,
            SCMP_CMP(0, SCMP_CMP_EQ, fd),
            SCMP_CMP(1, SCMP_CMP_EQ, TCGETS));
#ifdef TCGETS2
        seccomp_rule_add(
            ctx,
            SCMP_ACT_ALLOW,
            SCMP_SYS(ioctl),
            2,
            SCMP_CMP(0, SCMP_CMP_EQ, fd),
            SCMP_CMP(1, SCMP_CMP_EQ, TCGETS2));
#endif
    }

    // apply rules
    seccomp_load(ctx);
    seccomp_release(ctx);
}
#endif

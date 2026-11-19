#include "passgen.h"
#include "prompt.h"
#include "tomlc17.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
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

    // a wordlist which could not be read is found by its name, to say which
    // file it is. it may be in a preset.
    for(const passgen_error *current = error; current;
        current = current->cause) {
        if(current->kind != PASSGEN_ERROR_WORDLIST_READ) {
            continue;
        }

        const char *path = passgen_env_wordlist_path(env, current->detail);
        if(path) {
            fprintf(
                stderr,
                "\033[1;33mwordlist %s: \033[0m%s\n",
                current->detail,
                path);
        }
    }
}

static int passgen_cli_presets_list(const passgen_cli_opts *opts);

static passgen_cli_master *
passgen_cli_master_find(const passgen_cli_opts *opts, const char *name);

#ifdef PASSGEN_MONOCYPHER
// Zeroes memory which held a secret, in a way which the compiler does not
// optimize away, as it does not know what writing through a volatile pointer
// does.
static void passgen_cli_wipe(void *data, size_t size) {
    volatile uint8_t *bytes = data;
    for(size_t i = 0; i < size; i++) {
        bytes[i] = 0;
    }
}

// Longest master passphrase which can be entered, in bytes.
#define PASSGEN_CLI_PASSPHRASE_MAX 1024

// Reads a master passphrase. Prints what went wrong, if anything.
static int passgen_cli_passphrase(const char *prompt, char *passphrase) {
    passgen_cli_prompt_status status = passgen_cli_prompt_secret(
        prompt,
        passphrase,
        PASSGEN_CLI_PASSPHRASE_MAX + 1);
    const char *problem = NULL;
    if(status == PASSGEN_CLI_PROMPT_EOF) {
        problem = "no master passphrase was entered";
    } else if(status == PASSGEN_CLI_PROMPT_TOO_LONG) {
        problem = "the master passphrase is too long";
    } else if(!passphrase[0]) {
        problem = "the master passphrase is empty";
    }
    if(problem) {
        fprintf(stderr, "\033[1;31merror\033[0m: %s\n", problem);
        return PASSGEN_CLI_ERROR_MASTER;
    }
    return 0;
}

// Derives a master key, and prints what went wrong, if anything.
static int passgen_cli_master_derive(
    passgen_master *master,
    unsigned version,
    unsigned level,
    const char *passphrase,
    const uint8_t *seed,
    size_t seed_size) {
    passgen_status status = passgen_master_derive(
        master,
        version,
        level,
        passphrase,
        seed,
        seed_size);
    if(status == PASSGEN_ERROR_OUT_OF_MEMORY) {
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }
    if(status != PASSGEN_OK) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: cannot derive the master key\n");
        return PASSGEN_CLI_ERROR_MASTER;
    }
    return 0;
}

// Writes the hash of a master key in hex, with the given number of digits.
static void passgen_cli_master_hash_hex(
    const passgen_master *master,
    char *hex,
    size_t digits) {
    uint8_t hash[PASSGEN_MASTER_HASH_SIZE];
    passgen_master_hash(master, hash);
    for(size_t i = 0; i < digits; i++) {
        uint8_t byte = hash[i / 2];
        hex[i] = "0123456789abcdef"[i % 2 ? byte & 15 : byte >> 4];
    }
    hex[digits] = 0;
}

// Opens the source of randomness for the master passphrase which is asked
// for, after checking that it is the right one, if the configuration says
// what its hash starts with.
static int
passgen_cli_master_open(const passgen_cli_opts *opts, passgen_random **random) {
    const passgen_cli_master *config =
        passgen_cli_master_find(opts, opts->master);
    if(!config) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: there is no master passphrase `%s` in the "
            "configuration, see --master-setup\n",
            opts->master);
        return PASSGEN_CLI_ERROR_MASTER;
    }

    char passphrase[PASSGEN_CLI_PASSPHRASE_MAX + 1];
    int ret = passgen_cli_passphrase("master passphrase: ", passphrase);
    passgen_master master;
    if(ret == 0) {
        ret = passgen_cli_master_derive(
            &master,
            config->version,
            config->level,
            passphrase,
            config->seed,
            config->seed_size);
    }
    passgen_cli_wipe(passphrase, sizeof(passphrase));
    if(ret != 0) {
        return ret;
    }

    if(config->hash) {
        char hash[2 * PASSGEN_MASTER_HASH_SIZE + 1];
        passgen_cli_master_hash_hex(&master, hash, strlen(config->hash));
        if(0 != strcmp(hash, config->hash)) {
            passgen_master_wipe(&master);
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: wrong master passphrase for `%s`\n",
                config->name);
            return PASSGEN_CLI_ERROR_MASTER;
        }
    }

    *random = passgen_master_random_open(
        NULL,
        &master,
        opts->master_domain,
        opts->master_user);
    passgen_master_wipe(&master);
    if(!*random) {
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }
    return 0;
}

// Prints a key of a TOML table, which is quoted unless it is a bare key. It
// cannot have control characters.
static void passgen_cli_toml_key(const char *key) {
    bool bare = *key != 0;
    for(const char *c = key; *c; c++) {
        bare = bare && ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ||
                        (*c >= '0' && *c <= '9') || *c == '-' || *c == '_');
    }
    if(bare) {
        printf("%s", key);
        return;
    }
    printf("\"");
    for(const char *c = key; *c; c++) {
        if(*c == '"' || *c == '\\') {
            printf("\\");
        }
        printf("%c", *c);
    }
    printf("\"");
}

// Whether a name of a master passphrase can be used, which is any name
// without control characters (as it is printed as a key of a table in TOML).
static bool passgen_cli_master_name_valid(const char *name) {
    if(!*name) {
        return false;
    }
    for(const char *c = name; *c; c++) {
        if((unsigned char) *c < 0x20 || *c == 0x7f) {
            return false;
        }
    }
    return true;
}

// Asks a question at the terminal, until the answer is valid, which the
// function says. An empty answer is valid if there is a default. Returns 0, or
// an error when there is no more input.
static int passgen_cli_ask(
    const char *prompt,
    char *answer,
    size_t size,
    bool (*valid)(const char *answer),
    const char *invalid) {
    while(true) {
        passgen_cli_prompt_status status =
            passgen_cli_prompt_line(prompt, answer, size);
        if(status == PASSGEN_CLI_PROMPT_EOF) {
            fprintf(stderr, "\n");
            return PASSGEN_CLI_ERROR_MASTER;
        }
        if(status == PASSGEN_CLI_PROMPT_OK && valid(answer)) {
            return 0;
        }
        fprintf(stderr, "%s\n", invalid);
    }
}

static bool passgen_cli_answer_name(const char *answer) {
    return !*answer || passgen_cli_master_name_valid(answer);
}

static bool passgen_cli_answer_level(const char *answer) {
    return !*answer ||
           (answer[0] >= '1' && answer[0] <= '0' + PASSGEN_MASTER_LEVEL_MAX &&
            !answer[1]);
}

static bool passgen_cli_answer_yes(const char *answer) {
    return 0 == strcmp(answer, "y") || 0 == strcmp(answer, "Y") ||
           0 == strcmp(answer, "yes");
}

static bool passgen_cli_answer_yes_no(const char *answer) {
    return !*answer || passgen_cli_answer_yes(answer) ||
           0 == strcmp(answer, "n") || 0 == strcmp(answer, "N") ||
           0 == strcmp(answer, "no");
}

// The name of the master passphrase when none is given.
#define PASSGEN_CLI_MASTER_NAME_DEFAULT "main"

// Sets up a master passphrase: asks for what was not given as options (at a
// terminal), and for the master passphrase (twice, at a terminal), and prints
// what the configuration needs to use it.
static int passgen_cli_master_setup(const passgen_cli_opts *opts) {
    // what is not given as an option is asked for at a terminal, and is the
    // default otherwise.
    bool interactive = passgen_cli_prompt_interactive();

    char name[256];
    const char *chosen = opts->master_setup;
    if(!chosen && interactive) {
        fprintf(stderr, "What should this master passphrase be called?\n");
        try(passgen_cli_ask(
            "name [" PASSGEN_CLI_MASTER_NAME_DEFAULT "]: ",
            name,
            sizeof(name),
            passgen_cli_answer_name,
            "The name cannot have control characters."));
        chosen = name[0] ? name : PASSGEN_CLI_MASTER_NAME_DEFAULT;
        fprintf(stderr, "\n");
    }
    if(!chosen) {
        chosen = PASSGEN_CLI_MASTER_NAME_DEFAULT;
    }
    if(!passgen_cli_master_name_valid(chosen)) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: the name of a master passphrase cannot "
            "be empty or have control characters\n");
        return PASSGEN_CLI_ERROR_MASTER;
    }

    unsigned level = opts->master_setup_level;
    if(!level && interactive) {
        fprintf(
            stderr,
            "How hard should it be to crack? Higher is harder, but slower.\n"
            "  1: fast (64 MiB of memory)\n"
            "  2: recommended (256 MiB of memory)\n"
            "  3: strongest (1 GiB of memory)\n");
        char answer[16];
        try(passgen_cli_ask(
            "level [2]: ",
            answer,
            sizeof(answer),
            passgen_cli_answer_level,
            "Choose 1, 2 or 3."));
        level = answer[0] ? (unsigned) (answer[0] - '0') : 0;
        fprintf(stderr, "\n");
    }
    if(!level) {
        level = PASSGEN_MASTER_LEVEL_DEFAULT;
    }

    bool generate_seed = opts->master_setup_seed;
    if(!generate_seed && interactive) {
        fprintf(
            stderr,
            "Use a seed? Without one, the master passphrase alone generates\n"
            "your passphrases, on any computer. With one, the seed is needed\n"
            "too: keep it safe, and back it up.\n");
        char answer[16];
        try(passgen_cli_ask(
            "use a seed? [y/N]: ",
            answer,
            sizeof(answer),
            passgen_cli_answer_yes_no,
            "Answer y or n."));
        generate_seed = passgen_cli_answer_yes(answer);
        fprintf(stderr, "\n");
    }

    uint8_t seed[16];
    size_t seed_size = 0;
    if(generate_seed) {
        passgen_random_read(opts->random, seed, sizeof(seed));
        if(passgen_random_failed(opts->random)) {
            return passgen_cli_bail(PASSGEN_CLI_ERROR_RANDOM_ALLOC, NULL);
        }
        seed_size = sizeof(seed);
    }

    if(interactive) {
        fprintf(
            stderr,
            "Choose a strong master passphrase. It is not stored, and cannot\n"
            "be recovered.\n");
    }
    char passphrase[PASSGEN_CLI_PASSPHRASE_MAX + 1];
    char again[PASSGEN_CLI_PASSPHRASE_MAX + 1];
    int ret = passgen_cli_passphrase("master passphrase: ", passphrase);
    if(ret == 0 && interactive) {
        ret = passgen_cli_passphrase("master passphrase again: ", again);
        if(ret == 0 && 0 != strcmp(passphrase, again)) {
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: the master passphrases are not the "
                "same\n");
            ret = PASSGEN_CLI_ERROR_MASTER;
        }
    }
    passgen_cli_wipe(again, sizeof(again));

    passgen_master master;
    if(ret == 0) {
        ret = passgen_cli_master_derive(
            &master,
            PASSGEN_MASTER_VERSION,
            level,
            passphrase,
            seed,
            seed_size);
    }
    passgen_cli_wipe(passphrase, sizeof(passphrase));
    if(ret != 0) {
        passgen_cli_wipe(seed, sizeof(seed));
        return ret;
    }

    // a prefix of the hash is enough to tell when a master passphrase is
    // mistyped, and says less about it.
    char hash[9];
    passgen_cli_master_hash_hex(&master, hash, 8);
    passgen_master_wipe(&master);

    // printed to the terminal, it still has to be put into the
    // configuration.
    if(passgen_cli_prompt_output_terminal()) {
        fprintf(
            stderr,
            "\nAdd this to ~/.config/passgen/config.toml, to use it with "
            "`passgen -m %s`:\n\n",
            chosen);
    }

    printf("[master.");
    passgen_cli_toml_key(chosen);
    printf("]\n");
    printf("version = %u\n", PASSGEN_MASTER_VERSION);
    printf("level = %u\n", level);
    if(seed_size) {
        printf("# keep this safe, and back it up\n");
        printf("seed = \"");
        for(size_t i = 0; i < seed_size; i++) {
            printf("%02x", seed[i]);
        }
        printf("\"\n");
    }
    printf("hash = \"%s\"\n", hash);
    passgen_cli_wipe(seed, sizeof(seed));

    return 0;
}
#endif

int passgen_cli_run(passgen_cli_opts opts) {
    if(opts.list) {
        return passgen_cli_presets_list(&opts);
    }
#ifdef PASSGEN_MONOCYPHER
    if(opts.master_setup_given) {
        return passgen_cli_master_setup(&opts);
    }
#endif

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

#ifdef PASSGEN_MONOCYPHER
    // the master passphrase is asked for once the pattern is known to be
    // fine.
    passgen_random *master = NULL;
    if(opts.master) {
        int ret = passgen_cli_master_open(&opts, &master);
        if(ret != 0) {
            passgen_pattern_free(pattern);
            return ret;
        }
        passgen_env_random_set(opts.env, master);
    }
#endif

    int ret = passgen_cli_generate(opts, pattern);
    passgen_pattern_free(pattern);

#ifdef PASSGEN_MONOCYPHER
    if(master) {
        passgen_env_random_set(opts.env, opts.random);
        passgen_random_free(master);
    }
#endif
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

static int passgen_cli_buffer_push(
    passgen_cli_buffer *buffer,
    const char *data,
    size_t len) {
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

static int
passgen_cli_buffer_puts(passgen_cli_buffer *buffer, const char *str) {
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
            snprintf(number, sizeof(number), "\",\"entropy\":%lf}", entropy);
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

    char *name = malloc(offset + 1);
    if(!name) {
        return PASSGEN_CLI_ERROR_ALLOC;
    }
    memcpy(name, input, offset);
    name[offset] = 0;

    // the file is only opened when a pattern uses the wordlist.
    passgen_status status =
        passgen_env_wordlist_add_path(opt->env, name, colon + 1);
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

static int
passgen_cli_presets_collect(void *user, const char *name, const char *pattern) {
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
#define PASSGEN_CLI_MASTER_OPTS "d:m:u:"

// options which only have a long form.
enum {
    PASSGEN_CLI_OPT_MASTER_SETUP = 256,
    PASSGEN_CLI_OPT_MASTER_LEVEL,
    PASSGEN_CLI_OPT_MASTER_SEED,
    PASSGEN_CLI_OPT_MASTER_VERSION,
};
#else
#define PASSGEN_CLI_MASTER_OPTS ""
#endif

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

    // if no pattern or preset was given, show help, unless the presets are
    // to be listed, or a master passphrase is set up.
    if(!opts->pattern && !opts->preset && !opts->list && !setup) {
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

    // these don't do anything on their own. most likely, the master
    // passphrase was forgotten.
    if(!opts->master && (opts->master_domain || opts->master_user)) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--domain and --user need --master");
    }

    // setting one up is something else than generating.
    if(setup && (opts->master || opts->pattern || opts->preset || opts->list)) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--master-setup cannot be used with a pattern, --preset, --list "
            "or --master");
    }
    if(!setup && (opts->master_setup_level || opts->master_setup_seed ||
                  opts->master_setup_version)) {
        return passgen_cli_bail(
            PASSGEN_CLI_ERROR_MASTER_CONFLICT,
            "--master-level, --master-seed and --master-version need "
            "--master-setup");
    }

    // the source of randomness is opened once the master passphrase is
    // entered, which is after the pattern is checked.
    if(opts->master) {
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
    "    -d, --domain DOMAIN    Domain for master passphrase mode\n"
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
        "    \\m{name:2}             The same, with a chain of length 2 (1 to 8,\n"
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

// Frees what a master passphrase from the configuration holds.
static void passgen_cli_master_free(passgen_cli_master *master) {
    free(master->name);
    free(master->hash);
    memset(master, 0, sizeof(*master));
}

void passgen_cli_opts_free(passgen_cli_opts *opts) {
    passgen_env_free(opts->env);
    free(opts->definitions);
    free(opts->configs);
    for(size_t i = 0; i < opts->masters_len; i++) {
        passgen_cli_master_free(&opts->masters[i]);
    }
    free(opts->masters);

    // the environment only borrows the source of randomness.
    if(opts->random) {
        passgen_random_free(opts->random);
    }
}

// the highest level of a master passphrase. the configuration is read the same
// way without master passphrase mode, so this does not come from it.
#define PASSGEN_CLI_MASTER_LEVEL_MAX 3

// where the configuration which is installed along with passgen is, and the
// one for the whole system. these are set when building, according to where
// it is installed.
#ifndef PASSGEN_CLI_CONFIG_DATA
#define PASSGEN_CLI_CONFIG_DATA "/usr/share/passgen/config.toml"
#endif
#ifndef PASSGEN_CLI_CONFIG_SYSTEM
#define PASSGEN_CLI_CONFIG_SYSTEM "/etc/passgen/config.toml"
#endif

// Prints an error about a configuration file, which says where in it the
// problem is, if it is about something in particular.
static int passgen_cli_config_error(
    const char *path,
    const toml_datum_t *datum,
    const char *format,
    ...) {
    fprintf(stderr, "\033[1;31merror\033[0m: in configuration `%s`: ", path);
    if(datum && datum->lineno) {
        fprintf(stderr, "(line %d) ", datum->lineno);
    }
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    return PASSGEN_CLI_ERROR_CONFIG;
}

// Adds the wordlists of a configuration file. A relative path is relative to
// the directory of the configuration file.
static int passgen_cli_config_wordlists(
    passgen_cli_opts *opts,
    const char *path,
    toml_datum_t table) {
    // how much of the path of the configuration file is its directory,
    // including the slash.
    const char *slash = strrchr(path, '/');
    size_t directory_len = slash ? (size_t) (slash - path) + 1 : 0;

    for(int i = 0; i < table.u.tab.size; i++) {
        const char *name = table.u.tab.key[i];
        toml_datum_t value = table.u.tab.value[i];

        if(value.type == TOML_BOOLEAN && !value.u.boolean) {
            passgen_env_wordlist_remove(opts->env, name);
            continue;
        }
        if(value.type != TOML_STRING) {
            return passgen_cli_config_error(
                path,
                &value,
                "wordlist `%s` has to be a path, or false",
                name);
        }

        const char *wordlist = value.u.s;
        size_t prefix = wordlist[0] == '/' ? 0 : directory_len;
        size_t length = prefix + strlen(wordlist) + 1;
        char *full = malloc(length);
        if(!full) {
            return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
        }
        memcpy(full, path, prefix);
        strcpy(&full[prefix], wordlist);

        passgen_status status =
            passgen_env_wordlist_add_path(opts->env, name, full);
        free(full);
        if(status != PASSGEN_OK) {
            return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
        }
    }

    return 0;
}

// Adds the presets of a configuration file.
static int passgen_cli_config_presets(
    passgen_cli_opts *opts,
    const char *path,
    toml_datum_t table) {
    for(int i = 0; i < table.u.tab.size; i++) {
        const char *name = table.u.tab.key[i];
        toml_datum_t value = table.u.tab.value[i];

        if(value.type == TOML_BOOLEAN && !value.u.boolean) {
            passgen_env_preset_remove(opts->env, name);
            continue;
        }
        if(value.type != TOML_STRING) {
            return passgen_cli_config_error(
                path,
                &value,
                "preset `%s` has to be a pattern, or false",
                name);
        }

        if(PASSGEN_OK != passgen_env_preset_add(opts->env, name, value.u.s)) {
            return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
        }
    }

    return 0;
}

// Finds a master passphrase from the configuration by its name.
static passgen_cli_master *
passgen_cli_master_find(const passgen_cli_opts *opts, const char *name) {
    for(size_t i = 0; i < opts->masters_len; i++) {
        if(0 == strcmp(opts->masters[i].name, name)) {
            return &opts->masters[i];
        }
    }
    return NULL;
}

// Whether a string is only hex digits, of which there are at least one and at
// most max.
static bool passgen_cli_hex(const char *string, size_t max) {
    size_t length = strlen(string);
    if(length < 1 || length > max) {
        return false;
    }
    for(size_t i = 0; i < length; i++) {
        if(!strchr("0123456789abcdefABCDEF", string[i])) {
            return false;
        }
    }
    return true;
}

// Reads one master passphrase of a configuration file, which is a table.
static int passgen_cli_config_master(
    const char *path,
    const char *name,
    toml_datum_t table,
    passgen_cli_master *master) {
    bool version = false, level = false;
    for(int i = 0; i < table.u.tab.size; i++) {
        const char *key = table.u.tab.key[i];
        toml_datum_t value = table.u.tab.value[i];

        if(0 == strcmp(key, "version")) {
            if(value.type != TOML_INT64 || value.u.int64 != 1) {
                return passgen_cli_config_error(
                    path,
                    &value,
                    "master passphrase `%s` has to have version 1",
                    name);
            }
            master->version = 1;
            version = true;
        } else if(0 == strcmp(key, "level")) {
            if(value.type != TOML_INT64 || value.u.int64 < 1 ||
               value.u.int64 > PASSGEN_CLI_MASTER_LEVEL_MAX) {
                return passgen_cli_config_error(
                    path,
                    &value,
                    "master passphrase `%s` has to have a level from 1 to 3",
                    name);
            }
            master->level = (unsigned) value.u.int64;
            level = true;
        } else if(0 == strcmp(key, "hash")) {
            if(value.type != TOML_STRING || !passgen_cli_hex(value.u.s, 64)) {
                return passgen_cli_config_error(
                    path,
                    &value,
                    "the hash of master passphrase `%s` has to be 1 to 64 hex "
                    "digits",
                    name);
            }
            master->hash = strdup(value.u.s);
            if(!master->hash) {
                return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
            }
            for(char *c = master->hash; *c; c++) {
                if(*c >= 'A' && *c <= 'F') {
                    *c = (char) (*c - 'A' + 'a');
                }
            }
        } else if(0 == strcmp(key, "seed")) {
            size_t digits = value.type == TOML_STRING ? strlen(value.u.s) : 0;
            if(value.type != TOML_STRING ||
               (digits &&
                (digits % 2 ||
                 !passgen_cli_hex(value.u.s, 2 * sizeof(master->seed))))) {
                return passgen_cli_config_error(
                    path,
                    &value,
                    "the seed of master passphrase `%s` has to be up to 64 "
                    "bytes in hex",
                    name);
            }
            for(size_t j = 0; j < digits / 2; j++) {
                unsigned byte;
                sscanf(&value.u.s[2 * j], "%2x", &byte);
                master->seed[j] = (uint8_t) byte;
            }
            master->seed_size = digits / 2;
        } else {
            return passgen_cli_config_error(
                path,
                &value,
                "unknown key `%s` in master passphrase `%s`",
                key,
                name);
        }
    }

    // these change what is derived, so they are not left to defaults, which
    // may change.
    if(!version || !level) {
        return passgen_cli_config_error(
            path,
            &table,
            "master passphrase `%s` needs a version and a level",
            name);
    }

    return 0;
}

// Adds the master passphrases of a configuration file, replacing ones with
// the same name, or removing them if they are set to false.
static int passgen_cli_config_masters(
    passgen_cli_opts *opts,
    const char *path,
    toml_datum_t table) {
    for(int i = 0; i < table.u.tab.size; i++) {
        const char *name = table.u.tab.key[i];
        toml_datum_t value = table.u.tab.value[i];
        passgen_cli_master *existing = passgen_cli_master_find(opts, name);

        if(value.type == TOML_BOOLEAN && !value.u.boolean) {
            if(existing) {
                passgen_cli_master_free(existing);
                *existing = opts->masters[--opts->masters_len];
            }
            continue;
        }
        if(value.type != TOML_TABLE) {
            return passgen_cli_config_error(
                path,
                &value,
                "master passphrase `%s` has to be a table, or false",
                name);
        }

        passgen_cli_master master;
        memset(&master, 0, sizeof(master));
        int ret = passgen_cli_config_master(path, name, value, &master);
        if(ret == 0) {
            master.name = strdup(name);
            if(!master.name) {
                ret = passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
            }
        }
        if(ret != 0) {
            passgen_cli_master_free(&master);
            return ret;
        }

        if(existing) {
            passgen_cli_master_free(existing);
            *existing = master;
            continue;
        }
        passgen_cli_master *masters = realloc(
            opts->masters,
            (opts->masters_len + 1) * sizeof(passgen_cli_master));
        if(!masters) {
            passgen_cli_master_free(&master);
            return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
        }
        opts->masters = masters;
        opts->masters[opts->masters_len++] = master;
    }

    return 0;
}

// Adds what a parsed configuration file defines.
static int passgen_cli_config_apply(
    passgen_cli_opts *opts,
    const char *path,
    toml_datum_t top) {
    for(int i = 0; i < top.u.tab.size; i++) {
        const char *key = top.u.tab.key[i];
        toml_datum_t value = top.u.tab.value[i];

        bool wordlists = 0 == strcmp(key, "wordlists");
        bool presets = 0 == strcmp(key, "presets");
        bool master = 0 == strcmp(key, "master");
        if(!wordlists && !presets && !master) {
            return passgen_cli_config_error(
                path,
                &value,
                "unknown key `%s`",
                key);
        }
        if(value.type != TOML_TABLE) {
            return passgen_cli_config_error(
                path,
                &value,
                "`%s` has to be a table",
                key);
        }

        if(wordlists) {
            try(passgen_cli_config_wordlists(opts, path, value));
        } else if(master) {
            try(passgen_cli_config_masters(opts, path, value));
        } else {
            try(passgen_cli_config_presets(opts, path, value));
        }
    }

    return 0;
}

int passgen_cli_opts_config_file(
    passgen_cli_opts *opts,
    const char *path,
    bool required) {
    errno = 0;
    FILE *file = fopen(path, "r");
    if(!file) {
        // one which does not exist is fine, unless it was asked for.
        if(!required && (errno == ENOENT || errno == ENOTDIR)) {
            return 0;
        }
        return passgen_cli_config_error(path, NULL, "%s", strerror(errno));
    }

    // TOML has to be valid UTF-8.
    toml_option_t option = toml_default_option();
    option.check_utf8 = true;
    toml_set_option(option);

    toml_result_t result = toml_parse_file(file);
    fclose(file);

    int ret = 0;
    if(!result.ok) {
        ret = passgen_cli_config_error(path, NULL, "%s", result.errmsg);
    } else {
        ret = passgen_cli_config_apply(opts, path, result.toptab);
    }

    toml_free(result);
    return ret;
}

// Loads the configuration of the user, which is in $XDG_CONFIG_HOME, or else
// in ~/.config. If neither is set, there is none.
static int passgen_cli_opts_config_user(passgen_cli_opts *opts) {
    const char *dir = getenv("XDG_CONFIG_HOME");
    const char *suffix = "/passgen/config.toml";
    if(!dir || !*dir) {
        dir = getenv("HOME");
        suffix = "/.config/passgen/config.toml";
        if(!dir || !*dir) {
            return 0;
        }
    }

    size_t length = strlen(dir) + strlen(suffix) + 1;
    char *path = malloc(length);
    if(!path) {
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }
    snprintf(path, length, "%s%s", dir, suffix);
    int ret = passgen_cli_opts_config_file(opts, path, false);
    free(path);
    return ret;
}

// Loads the configuration files in a list which is separated by colons, like
// $PATH. Empty entries are skipped, as are files which do not exist.
static int
passgen_cli_opts_config_list(passgen_cli_opts *opts, const char *list) {
    char *copy = malloc(strlen(list) + 1);
    if(!copy) {
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }
    strcpy(copy, list);

    int ret = 0;
    char *path = copy;
    while(ret == 0 && path) {
        char *colon = strchr(path, ':');
        if(colon) {
            *colon = 0;
        }
        if(*path) {
            ret = passgen_cli_opts_config_file(opts, path, false);
        }
        path = colon ? colon + 1 : NULL;
    }

    free(copy);
    return ret;
}

int passgen_cli_opts_config(passgen_cli_opts *opts) {
    // the ones which are given with --config have to exist.
    if(opts->configs_len) {
        for(size_t i = 0; i < opts->configs_len; i++) {
            try(passgen_cli_opts_config_file(opts, opts->configs[i], true));
        }
        return 0;
    }

    // set to nothing, this loads none.
    const char *list = getenv("PASSGEN_CONFIG");
    if(list) {
        return passgen_cli_opts_config_list(opts, list);
    }

    // what is installed along with passgen, what is set for the system, and
    // then what the user set, each of which can replace what came before.
    try(passgen_cli_opts_config_file(opts, PASSGEN_CLI_CONFIG_DATA, false));
    try(passgen_cli_opts_config_file(opts, PASSGEN_CLI_CONFIG_SYSTEM, false));
    return passgen_cli_opts_config_user(opts);
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

    // allow checking whether a file is a terminal (which only reads its
    // settings). the C library does this when it starts reading from or
    // writing to a character device, such as /dev/null or /dev/urandom, to
    // decide how to buffer: without this, `passgen a > /dev/null` and
    // `passgen -r file:/dev/urandom a` were killed. newer versions of glibc
    // use TCGETS2 on some architectures.
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(ioctl),
        1,
        SCMP_CMP(1, SCMP_CMP_EQ, TCGETS));
#ifdef TCGETS2
    seccomp_rule_add(
        ctx,
        SCMP_ACT_ALLOW,
        SCMP_SYS(ioctl),
        1,
        SCMP_CMP(1, SCMP_CMP_EQ, TCGETS2));
#endif

    // allow turning off showing what is typed in the terminal (and back on),
    // to read a master passphrase from standard input. newer versions of
    // glibc use TCSETS2 and friends on some architectures.
    const unsigned long tcsets[] = {
        TCSETS,
        TCSETSW,
        TCSETSF,
#ifdef TCSETS2
        TCSETS2,
        TCSETSW2,
        TCSETSF2,
#endif
    };
    for(size_t i = 0; i < sizeof(tcsets) / sizeof(tcsets[0]); i++) {
        seccomp_rule_add(
            ctx,
            SCMP_ACT_ALLOW,
            SCMP_SYS(ioctl),
            2,
            SCMP_CMP(0, SCMP_CMP_EQ, 0),
            SCMP_CMP(1, SCMP_CMP_EQ, tcsets[i]));
    }

    // allow setting what happens on being interrupted, which sets the
    // terminal back while reading a master passphrase.
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigaction), 0);

    // apply rules
    seccomp_load(ctx);
    seccomp_release(ctx);
}
#endif

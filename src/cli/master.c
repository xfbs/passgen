/// @file master.c
///
/// Master passphrase mode: setting up a master passphrase, and opening the
/// source of randomness for one which is asked for.
#include "passgen.h"
#include "prompt.h"
#include <stdio.h>
#include <string.h>

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
int passgen_cli_master_open(
    const passgen_cli_opts *opts,
    passgen_random **random) {
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
int passgen_cli_master_setup(const passgen_cli_opts *opts) {
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

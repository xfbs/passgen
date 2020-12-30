#include "passgen/wordlist.h"
#include "passgen/assert.h"
#include <stdlib.h>
#include <string.h>

void help(const char **args, size_t args_len) {
    (void) args;
    (void) args_len;
}

void load(passgen_wordlist_t *wordlist, const char *filename) {
    FILE *file = fopen(filename, "r");
    passgen_assert(file);

    passgen_wordlist_load(wordlist, file);
    fclose(file);
}

void dump(const char **args, size_t args_len) {
    passgen_assert(args_len == 3);
    const char *filename = args[2];
    passgen_assert(filename);

    passgen_wordlist_t wordlist;
    load(&wordlist, filename);

    printf("size = %zu\n", wordlist.size);
    printf("count = %zu\n", wordlist.count);

    for(size_t i = 0; i < wordlist.count; i++) {
        printf("%s\n", wordlist.words[i]);
    }

    passgen_wordlist_free(&wordlist);
}

void random_word(const char **args, size_t args_len) {
    passgen_assert(args_len == 3);
    const char *filename = args[2];
    passgen_assert(filename);

    passgen_random_t random;
    passgen_random_open(&random);
    passgen_wordlist_t wordlist;
    load(&wordlist, filename);

    const char *word = passgen_wordlist_random(&wordlist, &random);
    printf("%s\n", word);

    passgen_random_close(&random);
    passgen_wordlist_free(&wordlist);
}

typedef void subcommand(const char **args, size_t args_len);

struct subcommands {
    subcommand *cmd;
    const char *name;
};

struct subcommands subcommands[] = {
    {dump, "dump"},
    {random_word, "random"},
    {help, "help"},
    {NULL, NULL},
};

int main(int argc, char *argv[]) {
    if(argc <= 1) {
        help((const char **) &argv[0], argc);
        return EXIT_SUCCESS;
    }

    passgen_assert(argc > 1);

    for(size_t i = 0; subcommands[i].name; i++) {
        if(0 == strcmp(subcommands[i].name, argv[1])) {
            subcommands[i].cmd((const char **) &argv[0], argc);
            return EXIT_SUCCESS;
        }
    }

    fprintf(stderr, "Error: unrecognized subcommand '%s'.\n", argv[1]);
    return EXIT_FAILURE;
}

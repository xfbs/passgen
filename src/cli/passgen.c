/// @file passgen.c
///
/// Runs the command-line tool, once its options and configuration are loaded:
/// lists the presets, sets up a master passphrase, or parses the pattern and
/// generates from it.
#include "passgen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        // this copy of the options is only used for generating.
        opts.random = master;
    }
#endif

    int ret = passgen_cli_generate(opts, pattern);
    passgen_pattern_free(pattern);

#ifdef PASSGEN_MONOCYPHER
    if(master) {
        passgen_env_random_set(opts.env, NULL);
        passgen_random_free(master);
    }
#endif
    return ret;
}

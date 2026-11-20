/// @file config.c
///
/// Reads the configuration files, which define wordlists, presets, master
/// passphrases and logins.
#include "passgen.h"
#include "tomlc17.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Frees what a master passphrase from the configuration holds.
void passgen_cli_master_free(passgen_cli_master *master) {
    free(master->name);
    free(master->hash);
    memset(master, 0, sizeof(*master));
}

// Frees what a login from the configuration holds.
void passgen_cli_login_free(passgen_cli_login *login) {
    free(login->domain);
    free(login->user);
    free(login->master);
    free(login->preset);
    free(login->pattern);
    free(login->path);
    memset(login, 0, sizeof(*login));
}

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
        // a path, or a table with the path and the hash of the file.
        const char *wordlist = NULL;
        const char *hash = NULL;
        if(value.type == TOML_STRING) {
            wordlist = value.u.s;
        } else if(value.type == TOML_TABLE) {
            for(int j = 0; j < value.u.tab.size; j++) {
                const char *key = value.u.tab.key[j];
                toml_datum_t field = value.u.tab.value[j];
                if(0 == strcmp(key, "path") && field.type == TOML_STRING) {
                    wordlist = field.u.s;
                } else if(
                    0 == strcmp(key, "hash") && field.type == TOML_STRING) {
                    hash = field.u.s;
                } else {
                    return passgen_cli_config_error(
                        path,
                        &field,
                        "wordlist `%s` can only have a path and a hash, which "
                        "are strings",
                        name);
                }
            }
        }
        if(!wordlist) {
            return passgen_cli_config_error(
                path,
                &value,
                "wordlist `%s` has to be a path, a table with a path (and a "
                "hash), or false",
                name);
        }
        // without monocypher, it is not checked, but it has to be valid,
        // so that the configuration is the same.
        if(hash && !passgen_cli_hex(hash, 128)) {
            return passgen_cli_config_error(
                path,
                &value,
                "the hash of wordlist `%s` has to be 1 to 128 hex digits, as "
                "b2sum prints it",
                name);
        }

        size_t prefix = wordlist[0] == '/' ? 0 : directory_len;
        size_t length = prefix + strlen(wordlist) + 1;
        char *full = malloc(length);
        if(!full) {
            return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
        }
        memcpy(full, path, prefix);
        strcpy(&full[prefix], wordlist);

        passgen_status status =
            passgen_env_wordlist_add_path(opts->env, name, full, hash);
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
passgen_cli_master *
passgen_cli_master_find(const passgen_cli_opts *opts, const char *name) {
    for(size_t i = 0; i < opts->masters_len; i++) {
        if(0 == strcmp(opts->masters[i].name, name)) {
            return &opts->masters[i];
        }
    }
    return NULL;
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

// Copies a string of a login into where it goes, if it is one. Returns 0, or
// an error if it is not a string or there is no memory for it.
static int passgen_cli_config_login_string(
    const char *path,
    const char *key,
    toml_datum_t value,
    char **string) {
    if(value.type != TOML_STRING) {
        return passgen_cli_config_error(
            path,
            &value,
            "`%s` of a login has to be a string",
            key);
    }
    free(*string);
    *string = strdup(value.u.s);
    if(!*string) {
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }
    return 0;
}

// Reads one login of a configuration file, which is a table.
static int passgen_cli_config_login(
    const char *path,
    toml_datum_t table,
    passgen_cli_login *login) {
    for(int i = 0; i < table.u.tab.size; i++) {
        const char *key = table.u.tab.key[i];
        toml_datum_t value = table.u.tab.value[i];
        char **string = NULL;
        if(0 == strcmp(key, "domain")) {
            string = &login->domain;
        } else if(0 == strcmp(key, "user")) {
            string = &login->user;
        } else if(0 == strcmp(key, "master")) {
            string = &login->master;
        } else if(0 == strcmp(key, "preset")) {
            string = &login->preset;
        } else if(0 == strcmp(key, "pattern")) {
            string = &login->pattern;
        } else if(0 == strcmp(key, "offset")) {
            if(value.type != TOML_INT64 || value.u.int64 < 0) {
                return passgen_cli_config_error(
                    path,
                    &value,
                    "the offset of a login has to be a number, from 0");
            }
            login->offset = (uint64_t) value.u.int64;
            continue;
        } else {
            return passgen_cli_config_error(
                path,
                &value,
                "unknown key `%s` in a login",
                key);
        }
        try(passgen_cli_config_login_string(path, key, value, string));
    }

    if(!login->domain || !login->domain[0]) {
        return passgen_cli_config_error(path, &table, "a login needs a domain");
    }
    if(login->preset && login->pattern) {
        return passgen_cli_config_error(
            path,
            &table,
            "a login can have a preset or a pattern, not both");
    }
    if(!login->user) {
        login->user = strdup("");
    }
    login->path = strdup(path);
    if(!login->user || !login->path) {
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }
    return 0;
}

// Adds the logins of a configuration file, which is an array of tables (as
// `[[login]]`). A domain and a user can only have one login.
static int passgen_cli_config_logins(
    passgen_cli_opts *opts,
    const char *path,
    toml_datum_t array) {
    if(array.type != TOML_ARRAY) {
        return passgen_cli_config_error(
            path,
            &array,
            "`login` has to be an array of tables, as [[login]]");
    }

    for(int i = 0; i < array.u.arr.size; i++) {
        toml_datum_t table = array.u.arr.elem[i];
        if(table.type != TOML_TABLE) {
            return passgen_cli_config_error(
                path,
                &table,
                "`login` has to be an array of tables, as [[login]]");
        }

        passgen_cli_login login;
        memset(&login, 0, sizeof(login));
        int ret = passgen_cli_config_login(path, table, &login);
        if(ret != 0) {
            passgen_cli_login_free(&login);
            return ret;
        }

        for(size_t j = 0; j < opts->logins_len; j++) {
            const passgen_cli_login *other = &opts->logins[j];
            if(0 == strcmp(other->domain, login.domain) &&
               0 == strcmp(other->user, login.user)) {
                if(login.user[0]) {
                    passgen_cli_config_error(
                        path,
                        &table,
                        "there is another login for `%s` and user `%s`, in "
                        "`%s`",
                        login.domain,
                        login.user,
                        other->path);
                } else {
                    passgen_cli_config_error(
                        path,
                        &table,
                        "there is another login for `%s` without a user, in "
                        "`%s`",
                        login.domain,
                        other->path);
                }
                passgen_cli_login_free(&login);
                return PASSGEN_CLI_ERROR_CONFIG;
            }
        }

        passgen_cli_login *logins = realloc(
            opts->logins,
            (opts->logins_len + 1) * sizeof(passgen_cli_login));
        if(!logins) {
            passgen_cli_login_free(&login);
            return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
        }
        opts->logins = logins;
        opts->logins[opts->logins_len++] = login;
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
        bool login = 0 == strcmp(key, "login");
        if(login) {
            try(passgen_cli_config_logins(opts, path, value));
            continue;
        }
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

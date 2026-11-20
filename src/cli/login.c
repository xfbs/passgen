/// @file login.c
///
/// Logins, which say which master passphrase, preset or pattern and offset
/// a domain and user use.
#include "passgen.h"
#include <stdio.h>
#include <string.h>

#ifdef PASSGEN_MONOCYPHER
// Finds the login for the domain (and user, if it is given), and fills in
// what was not given on the command line: the master passphrase, the user,
// the preset or pattern, and the offset.
int passgen_cli_login_apply(passgen_cli_opts *opts) {
    const passgen_cli_login *found = NULL;
    const passgen_cli_login *without_user = NULL;
    size_t count = 0;
    for(size_t i = 0; i < opts->logins_len; i++) {
        const passgen_cli_login *login = &opts->logins[i];
        if(0 != strcmp(login->domain, opts->master_domain)) {
            continue;
        }
        if(opts->master_user && 0 != strcmp(login->user, opts->master_user)) {
            continue;
        }
        found = login;
        count += 1;
        if(!login->user[0]) {
            without_user = login;
        }
    }

    // without a user, the login without one is meant, or else the only one.
    if(without_user) {
        found = without_user;
    } else if(count > 1) {
        // with a master passphrase, it is the domain without a user.
        if(opts->master) {
            return 0;
        }
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: there is more than one login for `%s`, "
            "choose one with --user:",
            opts->master_domain);
        const char *separator = " ";
        for(size_t i = 0; i < opts->logins_len; i++) {
            if(0 == strcmp(opts->logins[i].domain, opts->master_domain)) {
                fprintf(stderr, "%s`%s`", separator, opts->logins[i].user);
                separator = ", ";
            }
        }
        fprintf(stderr, "\n");
        return PASSGEN_CLI_ERROR_MASTER;
    }

    if(!found) {
        // with a master passphrase, a login is not needed.
        if(opts->master) {
            return 0;
        }
        if(opts->master_user) {
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: there is no login for `%s` and user "
                "`%s` in the configuration, give --master\n",
                opts->master_domain,
                opts->master_user);
        } else {
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: there is no login for `%s` in the "
                "configuration, give --master\n",
                opts->master_domain);
        }
        return PASSGEN_CLI_ERROR_MASTER;
    }

    if(!opts->master) {
        opts->master = found->master;
        // without one, it is the only one there is.
        if(!opts->master && opts->masters_len == 1) {
            opts->master = opts->masters[0].name;
        }
        if(!opts->master) {
            fprintf(
                stderr,
                "\033[1;31merror\033[0m: the login for `%s` does not say which "
                "master passphrase it uses, and there is %s, give --master\n",
                opts->master_domain,
                opts->masters_len ? "more than one" : "none");
            return PASSGEN_CLI_ERROR_MASTER;
        }
    }
    if(!opts->master_user) {
        opts->master_user = found->user;
    }
    if(!opts->pattern && !opts->preset) {
        opts->pattern = found->pattern;
        opts->preset = found->preset;
    }
    if(!opts->offset_given) {
        opts->offset = found->offset;
    }

    if(!opts->pattern && !opts->preset) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: the login for `%s` has no preset or "
            "pattern, give one\n",
            opts->master_domain);
        return PASSGEN_CLI_ERROR_MASTER;
    }

    // the stream of the last sequence has to exist.
    if(opts->amount - 1 > UINT64_MAX - opts->offset) {
        fprintf(
            stderr,
            "\033[1;31merror\033[0m: the offset is too large for the amount\n");
        return PASSGEN_CLI_ERROR_ILLEGAL_OFFSET;
    }
    return 0;
}
#endif

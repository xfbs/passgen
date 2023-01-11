#include "passgen.h"

#define try(statement)            \
    do {                          \
        int ret = statement;      \
        if(ret != EXIT_SUCCESS) { \
            return ret;           \
        }                         \
    } while(0)

int main(int argc, char *argv[]) {
    int ret;
    passgen_cli_opts opts;

    // seccomp is a linux feature that allows us to load filters for the
    // syscalls we are allowed to issue. this means that even if there is a
    // memory unsafety bug that allows arbitrary code execution, this process is
    // still sandboxed and can only perform safe system calls.
#ifdef PASSGEN_SECCOMP
    passgen_cli_seccomp_init();
#endif

    // initialise data structures.
    passgen_cli_opts_init(&opts);

    // load values from a config file (located at /etc/passgen.conf, or
    // $HOME/.config/passgen.conf).
    try(passgen_cli_opts_config(&opts));

    // parse CLI options, which override the previously loaded configuration.
    ret = passgen_cli_opts_parse(&opts, argc, argv);
    if(ret == PASSGEN_SHOW_HELP) {
        passgen_cli_usage(argv[0]);
        return 0;
    } else if(ret == PASSGEN_SHOW_VERSION) {
        passgen_cli_show_version();
        return 0;
    } else if(ret != PASSGEN_ERROR_NONE) {
        return ret;
    }

    // run the cli commands and free memory.
    try(passgen_cli_run(opts));
    passgen_cli_opts_free(&opts);

    return EXIT_SUCCESS;
}

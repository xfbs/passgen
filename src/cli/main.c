#include "passgen.h"

#define check_success(value) \
    ret = value;             \
    if(ret != EXIT_SUCCESS) return ret

int main(int argc, char *argv[]) {
    int ret;
    passgen_cli_opts opts;

    // seccomp is a linux feature that allows us to load filters for the syscalls we are allowed
    // to issue. this means that even if there is a memory unsafety bug that allows arbitrary code
    // execution, this process is still sandboxed and can only perform safe system calls.
#ifdef PASSGEN_SECCOMP
    passgen_cli_seccomp_init();
#endif

    // initialise data structures.
    passgen_cli_opts_init(&opts);

    // load values from a config file (located at /etc/passgen.conf, or $HOME/.config/passgen.conf).
    check_success(passgen_cli_opts_config(&opts));

    // parse CLI options, which override the previously loaded configuration.
    check_success(passgen_cli_opts_parse(&opts, argc, argv));

    // run the cli commands and free memory.
    passgen_cli_run(opts);
    passgen_cli_opts_free(&opts);

    return EXIT_SUCCESS;
}

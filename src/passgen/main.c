#include "passgen.h"

#define check_success(value) \
    ret = value;             \
    if(ret != EXIT_SUCCESS) return ret

int main(int argc, char *argv[]) {
    int ret;
    passgen_opts opts;

#ifdef PASSGEN_SECCOMP
    passgen_seccomp_init();
#endif

    passgen_opts_init(&opts);
    check_success(passgen_opts_config(&opts));
    check_success(passgen_opts_parse(&opts, argc, argv));
    passgen_run(opts);
    passgen_opts_free(&opts);

    return EXIT_SUCCESS;
}

#include "passgen.h"

#define check_success(value) \
    ret = value; \
    if(ret != EXIT_SUCCESS) return ret

int main(int argc, char *argv[]) {
    int ret;
    passgen_opts opts;
    passgen_opts_init(&opts);

    // try parsing config files
    FILE *file = fopen("/etc/passgen.conf", "r");
    if(file) {
        check_success(passgen_opts_load_file(&opts, file));
        fclose(file);
    }

    check_success(passgen_opts_parse(&opts, argc, argv));

    passgen_run(opts);
    passgen_opts_free(&opts);

    return EXIT_SUCCESS;
}

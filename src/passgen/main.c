#include "passgen.h"

int main(int argc, char *argv[]) {
    int ret;
    passgen_opts opts;
    passgen_opts_init(&opts);
    ret = passgen_opts_parse(&opts, argc, argv);
    if(ret != 0) {
        return ret;
    }
    passgen_run(opts);
    passgen_opts_free(&opts);

    return EXIT_SUCCESS;
}

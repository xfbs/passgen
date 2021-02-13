#include "passgen.h"

int main(int argc, char *argv[]) {
    passgen_opts opts;
    passgen_opts_parse(&opts, argc, argv);
    passgen_run(opts);
    passgen_opts_free(&opts);

    return EXIT_SUCCESS;
}

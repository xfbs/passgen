#include "passgen/passgen2.h"

int main(int argc, char *argv[]) {
     passgen_opts opts = passgen2_optparse(argc, argv);
     passgen2_run(opts);

     return EXIT_SUCCESS;
}

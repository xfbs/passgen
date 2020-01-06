#include "passgen/passgen.h"

int main(int argc, char *argv[]) {
  passgen_opts opts = passgen_optparse(argc, argv);
  passgen_run(opts);

  return EXIT_SUCCESS;
}

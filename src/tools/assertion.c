#include "passgen/assert.h"
#include <string.h>

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    assert(argc == 2);
    assert(0 == strcmp(argv[1], "true"));

    return 0;
}

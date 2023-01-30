#include "passgen/version.h"
#include "tests.h"
#include <string.h>

test_result test_version_get(void) {
    passgen_version version = passgen_version_get();
    assert_eq(version.major, 1);
    assert_eq(version.minor, 0);
    assert_eq(version.patch, 0);

    return test_ok;
}

test_result test_version_compatible(void) {
    passgen_version version = passgen_version_get();
    assert(passgen_version_compatible(version));

    version.patch += 1;
    assert(!passgen_version_compatible(version));
    version.patch -= 1;

    version.minor += 1;
    assert(!passgen_version_compatible(version));
    version.minor -= 1;

    version.major += 1;
    assert(!passgen_version_compatible(version));
    version.major -= 1;

    return test_ok;
}

test_result test_version_debug(void) {
    passgen_is_debug();

    return test_ok;
}

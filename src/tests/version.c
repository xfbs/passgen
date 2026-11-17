#include "passgen/version.h"
#include "passgen/config.h"
#include "tests.h"
#include <stdio.h>
#include <string.h>

test_result test_version_get(void) {
    // the library is the one these headers belong to.
    passgen_version version = passgen_version_get();
    assert_eq(version.major, PASSGEN_VERSION_MAJOR);
    assert_eq(version.minor, PASSGEN_VERSION_MINOR);
    assert_eq(version.patch, PASSGEN_VERSION_PATCH);

    char string[32];
    snprintf(
        string,
        sizeof(string),
        "%d.%d.%d",
        (int) version.major,
        (int) version.minor,
        (int) version.patch);
    assert_streq(string, PASSGEN_VERSION_STRING);

    return test_ok;
}

test_result test_version_compatible(void) {
    passgen_version version = passgen_version_get();
    assert(passgen_version_compatible(version));

    // a newer version than what we have is never compatible.
    version.patch += 1;
    assert(!passgen_version_compatible(version));
    version.patch -= 1;

    version.minor += 1;
    assert(!passgen_version_compatible(version));
    version.minor -= 1;

    version.major += 1;
    assert(!passgen_version_compatible(version));
    version.major -= 1;

    // an older patch version is.
    if(version.patch > 0) {
        version.patch -= 1;
        assert(passgen_version_compatible(version));
        version.patch += 1;
    }

    // an older minor version is as well, but only from 1.0.0 on.
    if(version.minor > 0) {
        version.minor -= 1;
        assert(passgen_version_compatible(version) == (version.major > 0));
        version.minor += 1;
    }

    return test_ok;
}

test_result test_version_debug(void) {
    passgen_is_debug();

    return test_ok;
}

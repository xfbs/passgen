/// @file public.cpp
///
/// Uses the library from C++, with only the public headers, and linked
/// against the shared library. C++ looks for functions under names which are
/// mangled, unless they are declared as extern "C", so if the headers do not
/// do that, this does not link. It only includes passgen.h, which has to be
/// enough for all of the interface.
#include <passgen/passgen.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define check(condition)                                                     \
    do {                                                                     \
        if(!(condition)) {                                                   \
            std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            std::exit(EXIT_FAILURE);                                         \
        }                                                                    \
    } while(0)

int main() {
    check(passgen_version_get().major == PASSGEN_VERSION_MAJOR);
    passgen_allocator_set(nullptr);

    passgen_pattern *pattern = nullptr;
    passgen_error error;
    check(PASSGEN_ERROR_SET_UNCLOSED == passgen_parse(&pattern, &error, "[a-z"));
    check(0 == std::strcmp(passgen_status_string(error.kind), "set is not closed"));
    passgen_error_free(&error);

    check(PASSGEN_OK == passgen_parse(&pattern, nullptr, "[a-z]{8}"));

    passgen_random random;
    check(passgen_random_open(&random, "xorshift:1234") != nullptr);
    passgen_env *env = passgen_env_new(&random);
    check(env != nullptr);

    uint8_t output[16];
    size_t length = 0;
    check(
        PASSGEN_OK == passgen_generate_fill_utf8(
                          pattern,
                          env,
                          nullptr,
                          nullptr,
                          output,
                          sizeof(output),
                          &length));
    check(length == 8);

    passgen_env_free(env);
    passgen_random_close(&random);
    passgen_pattern_free(pattern);

    return EXIT_SUCCESS;
}

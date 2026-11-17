/// @file error.c
///
/// Tests for errors: that what is wrong with a pattern is reported as such,
/// along with where it is, and that errors can be chained.
#include "passgen/util/error.h"
#include "tests.h"
#include <passgen/error.h>
#include <passgen/passgen.h>
#include <stdlib.h>
#include <string.h>

/// Every kind of mistake in a pattern results in its own status, with the
/// position of what is wrong.
test_result test_error_parse(void) {
    struct {
        const char *pattern;
        passgen_status status;
        // in codepoints
        size_t start;
        size_t end;
    } cases[] = {
        {"abc\\q", PASSGEN_ERROR_ESCAPE, 3, 5},
        {"[\\q]", PASSGEN_ERROR_ESCAPE, 1, 3},
        {"abc\\", PASSGEN_ERROR_ESCAPE_UNFINISHED, 4, 4},
        {"a\\u{12", PASSGEN_ERROR_ESCAPE_UNFINISHED, 6, 6},
        {"a\\uxyz", PASSGEN_ERROR_UNICODE_ESCAPE, 1, 4},
        {"a\\u{xyz}", PASSGEN_ERROR_UNICODE_ESCAPE, 1, 5},
        {"a\\u{1234567}", PASSGEN_ERROR_UNICODE_ESCAPE, 1, 11},
        {"a\\u{110000}b", PASSGEN_ERROR_UNICODE_RANGE, 1, 11},
        {"abc)", PASSGEN_ERROR_GROUP_UNOPENED, 3, 4},
        {"(abc", PASSGEN_ERROR_GROUP_UNCLOSED, 4, 4},
        {"((a)", PASSGEN_ERROR_GROUP_UNCLOSED, 4, 4},
        {"[abc", PASSGEN_ERROR_SET_UNCLOSED, 4, 4},
        {"[a-", PASSGEN_ERROR_SET_UNCLOSED, 3, 3},
        {"a[]b", PASSGEN_ERROR_SET_EMPTY, 2, 3},
        {"[z-a]", PASSGEN_ERROR_SET_RANGE, 3, 4},
        {"[a-]", PASSGEN_ERROR_SET_RANGE, 3, 4},
        {"a{5,2}", PASSGEN_ERROR_REPEAT, 5, 6},
        {"a{x}", PASSGEN_ERROR_REPEAT, 2, 3},
        {"a{2,x}", PASSGEN_ERROR_REPEAT, 4, 5},
        {"a{3", PASSGEN_ERROR_REPEAT, 3, 3},
        {"{x}a", PASSGEN_ERROR_REPEAT, 1, 2},
        {"a{99999999999999999999999}", PASSGEN_ERROR_REPEAT, 21, 22},
        {"?a", PASSGEN_ERROR_OPTIONAL, 0, 1},
        {"(?a)", PASSGEN_ERROR_OPTIONAL, 1, 2},
        {"\\wabc", PASSGEN_ERROR_SPECIAL, 2, 3},
        {"\\w{abc", PASSGEN_ERROR_SPECIAL, 6, 6},
        {"\\m", PASSGEN_ERROR_SPECIAL, 2, 2},
        // the first mistake is what is reported
        {"a)[", PASSGEN_ERROR_GROUP_UNOPENED, 1, 2},
        {"[]\\q", PASSGEN_ERROR_SET_EMPTY, 1, 2},
        // positions are in codepoints, not bytes
        {"g\xc4\x99\xc5\x9b[", PASSGEN_ERROR_SET_UNCLOSED, 4, 4},
        {"g\xc4\x99\xc5\x9b)", PASSGEN_ERROR_GROUP_UNOPENED, 3, 4},
        // not valid UTF-8, after two characters which are
        {"ab\xff\xfe", PASSGEN_ERROR_UTF8, 2, 3},
        {NULL, PASSGEN_OK, 0, 0},
    };

    for(size_t i = 0; cases[i].pattern; i++) {
        passgen_pattern *pattern;
        passgen_error error;
        passgen_status status = passgen_parse(&pattern, &error, cases[i].pattern);

        if(status != cases[i].status || error.kind != cases[i].status ||
           !error.has_position ||
           error.start.codepoint != cases[i].start ||
           error.end.codepoint != cases[i].end) {
            fprintf(
                stderr,
                "\npattern '%s': expected '%s' at %zu to %zu, got '%s' at "
                "%zu to %zu\n",
                cases[i].pattern,
                passgen_status_string(cases[i].status),
                cases[i].start,
                cases[i].end,
                passgen_status_string(status),
                error.start.codepoint,
                error.end.codepoint);
        }

        assert(status == cases[i].status);
        assert(error.kind == cases[i].status);
        assert(error.has_position);
        assert(error.start.codepoint == cases[i].start);
        assert(error.end.codepoint == cases[i].end);
        assert(error.cause == NULL);
        assert(pattern == NULL);

        // the same status is returned without an error to fill in.
        assert(passgen_parse(&pattern, NULL, cases[i].pattern) == cases[i].status);

        passgen_error_free(&error);
    }

    return test_ok;
}

/// The position is given in bytes as well.
test_result test_error_parse_bytes(void) {
    passgen_pattern *pattern;
    passgen_error error;

    // every one of the first three characters is two bytes long.
    const char *string = "\xc4\x85\xc4\x99\xc5\x9b)";
    assert(PASSGEN_ERROR_GROUP_UNOPENED == passgen_parse(&pattern, &error, string));
    assert(error.start.codepoint == 3);
    assert(error.start.byte == 6);
    assert(error.end.codepoint == 4);
    assert(error.end.byte == 7);
    passgen_error_free(&error);

    // an escape sequence is a single token, which starts at the backslash.
    assert(PASSGEN_ERROR_ESCAPE == passgen_parse(&pattern, &error, "\xc4\x85\\q"));
    assert(error.start.codepoint == 1);
    assert(error.start.byte == 2);
    assert(error.end.codepoint == 3);
    assert(error.end.byte == 4);
    passgen_error_free(&error);

    return test_ok;
}

/// A pattern which is fine does not touch the error.
test_result test_error_parse_success(void) {
    passgen_pattern *pattern;
    passgen_error error;
    passgen_error_init(&error);
    error.kind = PASSGEN_ERROR_INTERNAL;

    assert(PASSGEN_OK == passgen_parse(&pattern, &error, "[a-z]{4}"));
    assert(pattern != NULL);
    assert(error.kind == PASSGEN_ERROR_INTERNAL);
    passgen_pattern_free(pattern);

    return test_ok;
}

/// Every status has a description of its own.
test_result test_error_status_string(void) {
    for(int a = PASSGEN_OK; a <= PASSGEN_ERROR_DEPTH_LIMIT; a++) {
        const char *string = passgen_status_string((passgen_status) a);
        assert(string != NULL);
        assert(strlen(string) > 0);
        assert(0 != strcmp(string, "unknown error"));

        for(int b = a + 1; b <= PASSGEN_ERROR_DEPTH_LIMIT; b++) {
            assert(0 != strcmp(string, passgen_status_string((passgen_status) b)));
        }
    }

    assert_streq(passgen_status_string(PASSGEN_OK), "success");
    assert_streq(passgen_status_string((passgen_status) 9999), "unknown error");

    return test_ok;
}

/// Reads back what was printed to a file.
static bool printed(const passgen_error *error, const char *expected) {
    FILE *file = tmpfile();
    if(!file) {
        return false;
    }

    passgen_error_print(error, file);

    char buffer[512] = {0};
    rewind(file);
    size_t length = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[length] = 0;
    fclose(file);

    if(0 != strcmp(buffer, expected)) {
        fprintf(stderr, "\nexpected:\n%s\ngot:\n%s\n", expected, buffer);
        return false;
    }

    return true;
}

/// Errors can have a detail, and a chain of causes.
test_result test_error_chain(void) {
    passgen_error error;

    // nothing wrong
    passgen_error_init(&error);
    assert(error.kind == PASSGEN_OK);
    assert(error.cause == NULL);
    assert(printed(&error, "success\n"));
    passgen_error_free(&error);

    // a single error, with and without a detail
    assert(PASSGEN_ERROR_SET_EMPTY == passgen_error_set(&error, PASSGEN_ERROR_SET_EMPTY));
    assert(!error.has_position);
    assert(printed(&error, "set is empty\n"));

    passgen_error_detail_set(&error, "english");
    assert(printed(&error, "set is empty: english\n"));

    // adding context to an error turns it into the cause.
    assert(
        PASSGEN_ERROR_INTERNAL ==
        passgen_error_wrap(&error, PASSGEN_ERROR_INTERNAL, &error));
    assert(error.kind == PASSGEN_ERROR_INTERNAL);
    assert(error.detail[0] == 0);
    assert(error.cause != NULL);
    assert(error.cause->kind == PASSGEN_ERROR_SET_EMPTY);
    assert_streq(error.cause->detail, "english");
    assert(error.cause->cause == NULL);
    assert(printed(&error, "internal error\ncaused by: set is empty: english\n"));

    // and again, for a chain of three.
    passgen_error_detail_set(&error, "middle");
    passgen_error_wrap(&error, PASSGEN_ERROR_OUT_OF_MEMORY, &error);
    assert(error.cause->cause != NULL);
    assert(error.cause->cause->cause == NULL);
    assert(printed(
        &error,
        "out of memory\n"
        "caused by: internal error: middle\n"
        "caused by: set is empty: english\n"));

    // this frees the causes, and leaves the error as nothing wrong.
    passgen_error_free(&error);
    assert(error.kind == PASSGEN_OK);
    assert(error.cause == NULL);

    // the cause can be another error, which is taken over.
    passgen_error inner;
    passgen_error_set(&inner, PASSGEN_ERROR_UTF8);
    passgen_error_wrap(&error, PASSGEN_ERROR_ESCAPE, &inner);
    assert(inner.kind == PASSGEN_OK);
    assert(error.cause != NULL);
    assert(error.cause->kind == PASSGEN_ERROR_UTF8);
    passgen_error_free(&error);
    passgen_error_free(&inner);

    // something which is not an error is not a cause.
    passgen_error_init(&inner);
    passgen_error_wrap(&error, PASSGEN_ERROR_ESCAPE, &inner);
    assert(error.cause == NULL);
    passgen_error_wrap(&error, PASSGEN_ERROR_ESCAPE, NULL);
    assert(error.cause == NULL);
    passgen_error_free(&error);

    return test_ok;
}

/// Details which do not fit are cut off, rather than overflowing.
test_result test_error_detail_long(void) {
    passgen_error error;
    passgen_error_set(&error, PASSGEN_ERROR_INTERNAL);

    char name[200];
    memset(name, 'x', sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;

    passgen_error_detail_set(&error, name);
    assert(strlen(error.detail) == sizeof(error.detail) - 1);
    assert(0 == strncmp(error.detail, name, sizeof(error.detail) - 1));

    passgen_error_detail_set(&error, NULL);
    assert(error.detail[0] == 0);

    passgen_error_free(&error);

    return test_ok;
}

/// Everything accepts not being given an error to fill in.
test_result test_error_null(void) {
    assert(PASSGEN_ERROR_UTF8 == passgen_error_set(NULL, PASSGEN_ERROR_UTF8));
    passgen_error_detail_set(NULL, "nothing");
    passgen_error_free(NULL);

    passgen_error_offset offset = {1, 2};
    assert(
        PASSGEN_ERROR_UTF8 ==
        passgen_error_set_at(NULL, PASSGEN_ERROR_UTF8, offset, offset));

    // the cause is released, as there is nowhere for it to go.
    passgen_error inner, cause;
    passgen_error_set(&cause, PASSGEN_ERROR_UTF8);
    passgen_error_wrap(&inner, PASSGEN_ERROR_ESCAPE, &cause);
    assert(inner.cause != NULL);
    assert(PASSGEN_ERROR_INTERNAL == passgen_error_wrap(NULL, PASSGEN_ERROR_INTERNAL, &inner));
    assert(inner.kind == PASSGEN_OK);
    assert(inner.cause == NULL);

    return test_ok;
}

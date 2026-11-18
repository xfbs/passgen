/// @file error.c
///
/// Tests for errors: that what is wrong with a pattern is reported as such,
/// along with where it is, and that errors can be chained.
#include "passgen/util/error.h"
#include "passgen/wordlist.h"
#include "testalloc.h"
#include "tests.h"
#include <passgen/env.h>
#include <passgen/error.h>
#include <passgen/generate.h>
#include <passgen/random.h>
#include <passgen/passgen.h>
#include <stdlib.h>
#include <string.h>

// pipes are used as files which cannot be read.
#ifndef _WIN32
#include <unistd.h>
#endif

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
        {"a\\w{}", PASSGEN_ERROR_SPECIAL, 4, 5},
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
    for(int a = PASSGEN_OK; a <= PASSGEN_ERROR_ABORTED; a++) {
        const char *string = passgen_status_string((passgen_status) a);
        assert(string != NULL);
        assert(strlen(string) > 0);
        assert(0 != strcmp(string, "unknown error"));

        for(int b = a + 1; b <= PASSGEN_ERROR_ABORTED; b++) {
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

static int count_callback(void *data, uint32_t codepoint) {
    (void) codepoint;
    *((size_t *) data) += 1;
    return 0;
}

/// Stops after a number of codepoints.
static int stopping_callback(void *data, uint32_t codepoint) {
    (void) codepoint;
    size_t *remaining = data;
    if(*remaining == 0) {
        return 1;
    }
    *remaining -= 1;
    return 0;
}

/// Every way in which generating can fail has a status of its own, and
/// errors with wordlists say which one it is.
test_result test_error_generate(void) {
    passgen_random random;
    assert(passgen_random_open(&random, "zero"));
    passgen_env *env = passgen_env_new(&random);
    assert(env);

    // an empty wordlist
    FILE *file = tmpfile();
    assert(file);
    assert(0 == passgen_env_wordlist_add(env, "empty", file, 2));

    passgen_pattern *pattern;
    passgen_error error;
    size_t count = 0;

    // a wordlist which is not there, as a word and as a markov chain
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "ab\\w{missing}"));
    assert(
        PASSGEN_ERROR_WORDLIST_NOT_FOUND ==
        passgen_generate(pattern, env, &error, NULL, &count, count_callback));
    assert(error.kind == PASSGEN_ERROR_WORDLIST_NOT_FOUND);
    assert(!error.has_position);
    assert_streq(error.detail, "missing");
    assert(error.cause == NULL);
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\m{missing}"));
    assert(
        PASSGEN_ERROR_WORDLIST_NOT_FOUND ==
        passgen_generate(pattern, env, &error, NULL, &count, count_callback));
    assert_streq(error.detail, "missing");
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    // a wordlist which is empty
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\w{empty}"));
    assert(
        PASSGEN_ERROR_WORDLIST_EMPTY ==
        passgen_generate(pattern, env, &error, NULL, &count, count_callback));
    assert_streq(error.detail, "empty");
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\m{empty}"));
    assert(
        PASSGEN_ERROR_WORDLIST_EMPTY ==
        passgen_generate(pattern, env, &error, NULL, &count, count_callback));
    assert_streq(error.detail, "empty");
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    // a group in which every segment has a multiplier of zero
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "{0}a"));
    assert(
        PASSGEN_ERROR_GROUP_EMPTY ==
        passgen_generate(pattern, env, &error, NULL, &count, count_callback));
    assert(error.kind == PASSGEN_ERROR_GROUP_EMPTY);
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    // nested more deeply than the limit
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "((a))"));
    passgen_env_depth_limit_set(env, 2);
    assert(
        PASSGEN_ERROR_DEPTH_LIMIT ==
        passgen_generate(pattern, env, &error, NULL, &count, count_callback));
    assert(error.kind == PASSGEN_ERROR_DEPTH_LIMIT);
    passgen_error_free(&error);
    passgen_env_depth_limit_set(env, 3);
    assert(
        PASSGEN_OK ==
        passgen_generate(pattern, env, &error, NULL, &count, count_callback));
    passgen_pattern_free(pattern);

    // the callback asking to stop
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "abcdef"));
    size_t remaining = 3;
    assert(
        PASSGEN_ERROR_ABORTED ==
        passgen_generate(pattern, env, &error, NULL, &remaining, stopping_callback));
    assert(error.kind == PASSGEN_ERROR_ABORTED);
    passgen_error_free(&error);

    // the error is not needed to find out what went wrong.
    remaining = 3;
    assert(
        PASSGEN_ERROR_ABORTED ==
        passgen_generate(pattern, env, NULL, NULL, &remaining, stopping_callback));

    // the output not fitting. this is not the same as nothing being
    // generated, which is fine.
    uint8_t utf8[8];
    uint32_t codepoints[8];
    size_t written = 99;
    assert(
        PASSGEN_ERROR_BUFFER_TOO_SMALL ==
        passgen_generate_fill_utf8(pattern, env, &error, NULL, utf8, 5, &written));
    assert(error.kind == PASSGEN_ERROR_BUFFER_TOO_SMALL);
    assert(written == 0);
    passgen_error_free(&error);

    written = 99;
    assert(
        PASSGEN_ERROR_BUFFER_TOO_SMALL ==
        passgen_generate_fill_unicode(pattern, env, NULL, NULL, codepoints, 5, &written));
    assert(written == 0);
    assert(
        PASSGEN_ERROR_BUFFER_TOO_SMALL ==
        passgen_generate_fill_json_utf8(pattern, env, NULL, NULL, utf8, 5, &written));

    // it fits exactly, and the length does not have to be asked for.
    assert(
        PASSGEN_OK ==
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, utf8, 6, &written));
    assert(written == 6);
    assert(
        PASSGEN_OK ==
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, utf8, 6, NULL));
    passgen_pattern_free(pattern);

    // nothing being generated
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, ""));
    written = 99;
    assert(
        PASSGEN_OK ==
        passgen_generate_fill_utf8(pattern, env, &error, NULL, utf8, 0, &written));
    assert(written == 0);
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_close(&random);

    return test_ok;
}

/// Counts down, and fails once it has given out all it has.
static size_t limited_read(void *context, void *dest, size_t size) {
    size_t *remaining = context;
    size_t count = size < *remaining ? size : *remaining;
    memset(dest, 0xaa, count);
    *remaining -= count;
    return count;
}

static void limited_close(void *context) {
    (void) context;
}

/// A source of randomness which runs out fails generating, rather than
/// aborting or generating something from what is left in the buffer.
test_result test_error_generate_random(void) {
    // enough for the first buffer and one more.
    size_t remaining = 2 * PASSGEN_RANDOM_BUFFER_LENGTH;
    passgen_random random = {
        .context = &remaining,
        .read = limited_read,
        .close = limited_close,
    };
    passgen_random_reload(&random);
    assert(!passgen_random_failed(&random));

    passgen_env *env = passgen_env_new(&random);
    assert(env);

    passgen_pattern *pattern;
    passgen_error error;
    uint8_t output[512];
    size_t written = 99;

    // every character takes eight bytes, so this is fine.
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "[a-z]{100}"));
    assert(
        PASSGEN_OK ==
        passgen_generate_fill_utf8(pattern, env, &error, NULL, output, 512, &written));
    assert(written == 100);
    assert(!passgen_random_failed(&random));
    passgen_pattern_free(pattern);

    // this needs more than there is.
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "[a-z]{300}"));
    assert(
        PASSGEN_ERROR_RANDOM ==
        passgen_generate_fill_utf8(pattern, env, &error, NULL, output, 512, &written));
    assert(error.kind == PASSGEN_ERROR_RANDOM);
    assert(written == 0);
    assert(passgen_random_failed(&random));
    assert(remaining == 0);
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    // it stays that way, even for choices which do not result in output, and
    // only gives out zeroes.
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "(a?){0,3}"));
    assert(
        PASSGEN_ERROR_RANDOM ==
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, output, 512, &written));
    passgen_pattern_free(pattern);

    assert(passgen_random_u64(&random) == 0);
    uint8_t large[2 * PASSGEN_RANDOM_BUFFER_LENGTH];
    memset(large, 1, sizeof(large));
    passgen_random_read(&random, large, sizeof(large));
    for(size_t i = 0; i < sizeof(large); i++) {
        assert(large[i] == 0);
    }

    // a pattern which does not need any randomness fails as well: the source
    // is not to be trusted.
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "abc"));
    assert(
        PASSGEN_ERROR_RANDOM ==
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, output, 512, &written));
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_close(&random);

    return test_ok;
}

/// Reading more than fits into the buffer goes straight to the source, and
/// fails in the same way.
test_result test_error_random_large(void) {
    size_t remaining = PASSGEN_RANDOM_BUFFER_LENGTH + 100;
    passgen_random random = {
        .context = &remaining,
        .read = limited_read,
        .close = limited_close,
    };
    passgen_random_reload(&random);
    assert(!passgen_random_failed(&random));

    uint8_t large[2 * PASSGEN_RANDOM_BUFFER_LENGTH];
    memset(large, 1, sizeof(large));
    passgen_random_read(&random, large, sizeof(large));
    assert(passgen_random_failed(&random));
    for(size_t i = 0; i < sizeof(large); i++) {
        assert(large[i] == 0);
    }

    passgen_random_close(&random);

    return test_ok;
}

/// Files which do not have enough in them to fill the buffer once are not
/// opened, rather than aborting.
test_result test_error_random_file_short(void) {
    passgen_random random;

    FILE *file = tmpfile();
    assert(file);
    assert(!passgen_random_file_open(&random, file));

    file = tmpfile();
    assert(file);
    for(size_t i = 0; i < PASSGEN_RANDOM_BUFFER_LENGTH - 1; i++) {
        fputc('a', file);
    }
    rewind(file);
    assert(!passgen_random_file_open(NULL, file));

    // just enough for the buffer, after which it fails.
    file = tmpfile();
    assert(file);
    for(size_t i = 0; i < PASSGEN_RANDOM_BUFFER_LENGTH; i++) {
        fputc('a', file);
    }
    rewind(file);
    assert(passgen_random_file_open(&random, file));
    for(size_t i = 0; i < PASSGEN_RANDOM_BUFFER_LENGTH - 1; i++) {
        assert(passgen_random_u8(&random) == 'a');
    }
    assert(!passgen_random_failed(&random));
    assert(passgen_random_u8(&random) == 0);
    assert(passgen_random_failed(&random));
    passgen_random_close(&random);

    return test_ok;
}

static int ignore_callback(void *data, uint32_t codepoint) {
    (void) data;
    (void) codepoint;
    return 0;
}

#ifndef _WIN32
/// A wordlist which cannot be read fails generating, with the reason as the
/// cause of the error. It fails in the same way when tried again.
test_result test_error_wordlist_read(void) {
    passgen_random random;
    assert(passgen_random_open(&random, "zero"));
    passgen_env *env = passgen_env_new(&random);
    assert(env);

    // a pipe cannot be rewound, which is how the size of the file is found.
    int fds[2];
    assert(0 == pipe(fds));
    assert(5 == write(fds[1], "word\n", 5));
    close(fds[1]);
    FILE *file = fdopen(fds[0], "r");
    assert(file);
    assert(PASSGEN_OK == passgen_env_wordlist_add(env, "piped", file, 2));

    const char *patterns[] = {"\\w{piped}", "\\m{piped}", "\\w{piped}"};
    for(size_t i = 0; i < 3; i++) {
        passgen_pattern *pattern;
        passgen_error error;
        assert(PASSGEN_OK == passgen_parse(&pattern, NULL, patterns[i]));
        assert(
            PASSGEN_ERROR_WORDLIST_READ ==
            passgen_generate(pattern, env, &error, NULL, NULL, ignore_callback));
        assert(error.kind == PASSGEN_ERROR_WORDLIST_READ);
        assert_streq(error.detail, "piped");
        assert(error.cause);
        assert(error.cause->kind == PASSGEN_ERROR_IO);
        assert(error.cause->detail[0] != 0);
        assert(error.cause->cause == NULL);
        passgen_error_free(&error);

        // without an error to fill in, the cause is not leaked.
        assert(
            PASSGEN_ERROR_WORDLIST_READ ==
            passgen_generate(pattern, env, NULL, NULL, NULL, ignore_callback));
        passgen_pattern_free(pattern);
    }

    passgen_env_free(env);
    passgen_random_close(&random);

    return test_ok;
}

/// Loading a wordlist directly reports the same thing, and leaves nothing
/// behind.
test_result test_error_wordlist_load(void) {
    int fds[2];
    assert(0 == pipe(fds));
    close(fds[1]);
    FILE *file = fdopen(fds[0], "r");
    assert(file);

    passgen_wordlist wordlist;
    passgen_error error;
    assert(PASSGEN_ERROR_IO == passgen_wordlist_load(&wordlist, &error, file, 3));
    assert(error.kind == PASSGEN_ERROR_IO);
    assert(error.detail[0] != 0);
    assert(!wordlist.parsed);
    assert(!wordlist.parsed_markov);
    assert(wordlist.data == NULL);
    assert(wordlist.words == NULL);
    assert(wordlist.count == 0);
    passgen_error_free(&error);

    // this closes the file.
    passgen_wordlist_free(&wordlist);

    return test_ok;
}
#endif

/// A word which is not valid UTF-8 fails generating when it is picked, and
/// says that this is why.
test_result test_error_wordlist_utf8(void) {
    passgen_random random;
    assert(passgen_random_open(&random, "zero"));
    passgen_env *env = passgen_env_new(&random);
    assert(env);

    FILE *file = tmpfile();
    assert(file);
    fputs("\xff\xfe\n", file);
    assert(PASSGEN_OK == passgen_env_wordlist_add(env, "broken", file, 2));

    passgen_pattern *pattern;
    passgen_error error;
    assert(PASSGEN_OK == passgen_parse(&pattern, NULL, "\\w{broken}"));
    assert(
        PASSGEN_ERROR_WORDLIST_READ ==
        passgen_generate(pattern, env, &error, NULL, NULL, ignore_callback));
    assert_streq(error.detail, "broken");
    assert(error.cause);
    assert(error.cause->kind == PASSGEN_ERROR_UTF8);
    passgen_error_free(&error);
    passgen_pattern_free(pattern);

    passgen_env_free(env);
    passgen_random_close(&random);

    return test_ok;
}

/// Generates from a pattern into the buffer, with the same randomness every
/// time. Returns the length, or zero if it failed.
static size_t generate_fixed(passgen_pattern *pattern, uint8_t *output, size_t size) {
    passgen_random random;
    if(!passgen_random_open(&random, "xorshift:1234")) {
        return 0;
    }
    passgen_env *env = passgen_env_new(&random);
    size_t written = 0;
    if(env) {
        passgen_generate_fill_utf8(pattern, env, NULL, NULL, output, size, &written);
        passgen_env_free(env);
    }
    passgen_random_close(&random);
    return written;
}

/// Running out of memory at any point while parsing fails with
/// PASSGEN_ERROR_OUT_OF_MEMORY, which has no position, and leaves nothing
/// allocated. Once there is enough memory, the pattern is the same as it
/// always is.
test_result test_error_parse_out_of_memory(void) {
    const char *patterns[] = {
        "",
        "abcdefghijklmnop",
        "a|b|c|d",
        "(a|b){2,5}[x-z]?",
        "[a-z0-9\\-\\]]{3}",
        "{3}abc|{2}def|ghi",
        "a?b?(c)?",
        "((((((nested))))))",
        "\\w{a-rather-long-wordlist-name}",
        "\\m{x}-\\w{y}",
        // more items, segments and ranges than fit into the first bin of a
        // stack.
        ("abcdefgh{2}abcdefgh{2}abcdefgh{2}abcdefgh{2}abcdefgh{2}abcdefgh{2}"
         "abcdefgh{2}abcdefgh{2}abcdefgh{2}abcdefgh{2}abcdefgh{2}abcdefgh{2}"),
        "a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z|0|1|2|3|4|5|6|7",
        "[aceg-ikmoqsuwy02468ACEGIKMOQSUWY!#%&()*+,./:;<=>?@^_`{|}~]",
    };

    for(size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        // what it is when nothing fails, for comparing.
        passgen_pattern *pattern;
        assert(PASSGEN_OK == passgen_parse(&pattern, NULL, patterns[i]));
        uint8_t expected[1024];
        size_t expected_len = 0;
        if(!strstr(patterns[i], "\\")) {
            expected_len = generate_fixed(pattern, expected, sizeof(expected));
        }
        passgen_pattern_free(pattern);

        bool done = false;
        for(size_t fail_at = 0; !done; fail_at++) {
            test_allocator allocator;
            test_allocator_install(&allocator, fail_at);

            passgen_error error;
            pattern = (passgen_pattern *) &error;
            passgen_status status = passgen_parse(&pattern, &error, patterns[i]);

            if(status == PASSGEN_OK) {
                done = true;
                if(!strstr(patterns[i], "\\")) {
                    uint8_t output[1024];
                    size_t output_len =
                        generate_fixed(pattern, output, sizeof(output));
                    assert_eq(output_len, expected_len);
                    assert(0 == memcmp(output, expected, output_len));
                }
                passgen_pattern_free(pattern);
            } else {
                assert_eq(status, PASSGEN_ERROR_OUT_OF_MEMORY);
                assert_eq(error.kind, PASSGEN_ERROR_OUT_OF_MEMORY);
                assert(!error.has_position);
                assert(pattern == NULL);
                passgen_error_free(&error);
            }

            assert(test_allocator_clean(&allocator));
            test_allocator_remove();
        }
    }

    return test_ok;
}

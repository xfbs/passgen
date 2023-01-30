#include "passgen/wordlist.h"
#include "tests.h"
#include <string.h>

const char example_wordlist[] = "abacus\nbrother\nculling";
size_t example_wordlist_size = sizeof(example_wordlist);

test_result test_wordlist_load(void) {
#ifndef _WIN32
    FILE *file =
        fmemopen((void *) example_wordlist, example_wordlist_size, "r");
    assert(file);

    passgen_wordlist wordlist;
    passgen_wordlist_load(&wordlist, file, 3);

    assert(wordlist.count == 3);
    assert(0 == strcmp(wordlist.words[0], "abacus"));
    assert(0 == strcmp(wordlist.words[1], "brother"));
    assert(0 == strcmp(wordlist.words[2], "culling"));

    passgen_wordlist_free(&wordlist);
#endif

    return test_ok;
}

test_result test_wordlist_load_long(void) {
    char *wordlist_data = malloc(26 * 26 * 3 + 1);
    for(size_t a = 0; a < 26; a++) {
        for(char b = 0; b < 26; b++) {
            wordlist_data[a * 26 * 3 + b * 3] = 'a' + a;
            wordlist_data[a * 26 * 3 + b * 3 + 1] = 'a' + b;
            wordlist_data[a * 26 * 3 + b * 3 + 2] = '\n';
        }
    }

    // null-terminate
    wordlist_data[26 * 26 * 3 - 1] = 0;

#ifndef _WIN32
    FILE *file = fmemopen(
        (void *) wordlist_data,
        strlen(wordlist_data),
        "r");
    assert(file);

    passgen_wordlist wordlist;
    passgen_wordlist_load(&wordlist, file, 3);

    assert_eq(wordlist.count, 26 * 26);
    assert_eq(passgen_wordlist_count(&wordlist), 26 * 26);
    assert(0 == strcmp(wordlist.words[0], "aa"));
    assert(0 == strcmp(wordlist.words[1], "ab"));
    assert(0 == strcmp(wordlist.words[2], "ac"));
    assert(0 == strcmp(wordlist.words[26], "ba"));
    assert(0 == strcmp(wordlist.words[27], "bb"));
    assert(0 == strcmp(wordlist.words[28], "bc"));
    assert(0 == strcmp(wordlist.words[26 * 26 - 1], "zz"));

    passgen_wordlist_free(&wordlist);
#endif

    free(wordlist_data);

    return test_ok;
}

test_result test_wordlist_random(void) {
#ifndef _WIN32
    FILE *file =
        fmemopen((void *) example_wordlist, example_wordlist_size, "r");
    assert(file);

    passgen_wordlist wordlist;
    passgen_wordlist_load(&wordlist, file, 3);

    passgen_random random;
    passgen_random_open(&random, NULL);

    for(size_t i = 0; i < wordlist.count; i++) {
        const char *word = passgen_wordlist_random(&wordlist, &random);

        // make sure the word exists in the word list
        bool found = false;
        for(size_t j = 0; j < wordlist.count; j++) {
            if(word == wordlist.words[j]) {
                found = true;
                break;
            }
        }
        assert(found);
    }

    passgen_wordlist_free(&wordlist);
    passgen_random_close(&random);
#endif

    return test_ok;
}

test_result test_wordlist_random_uninit(void) {
#ifndef _WIN32
    FILE *file =
        fmemopen((void *) example_wordlist, example_wordlist_size, "r");
    assert(file);

    passgen_wordlist wordlist;
    passgen_wordlist_init(&wordlist, file, 3);
    assert_eq(wordlist.parsed, false);

    passgen_random random;
    passgen_random_open(&random, NULL);

    const char *word = passgen_wordlist_random(&wordlist, &random);
    assert_eq(word, NULL);

    passgen_wordlist_free(&wordlist);
    passgen_random_close(&random);
#endif

    return test_ok;
}

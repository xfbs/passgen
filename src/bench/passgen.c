#include <passgen/parser.h>
#include <passgen/assert.h>
#include <passgen/wordlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define REPETITIONS 100
#define GENERATIONS 10

int main(int argc, char *argv[]) {
    assert(argc == 2);

    const char *patterns_path = argv[1];
    assert(patterns_path);

    FILE *patterns_file = fopen(patterns_path, "r");
    assert(patterns_file);

    passgen_wordlist_t patterns;
    passgen_wordlist_load(&patterns, patterns_file);
    fclose(patterns_file);

    double unicode_time;
    double tokenize_time;
    double parse_time;
    double generate_time;
    double free_time;

    for(size_t i = 0; i < patterns.count; i++) {
        const char *pattern = patterns.words[i];
        assert(pattern);
        printf("%s\n", pattern);

        for(size_t r = 0; r < REPETITIONS; r++) {
            uint32_t format_decoded[256];
            size_t format_decoded_len = 0;
            size_t format_bytes_read = 0;
            size_t format_input_len = strlen(pattern);

            int ret = passgen_utf8_decode(
                format_decoded,
                256,
                &format_decoded_len,
                (unsigned char *) pattern,
                format_input_len,
                &format_bytes_read);

            assert(ret == 0);
        }
    }

    passgen_wordlist_free(&patterns);

    return EXIT_SUCCESS;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <passgen/util/utf8.h>
#include <passgen/parser/token.h>

int main(int argc, char *argv[]) {
    int ret;

    if(argc < 2) {
        printf("Usage: ./tokenize <string>\n");
        printf("Tokenizes the given string and prints out the individual tokens.\n");
        return 1;
    }

    // grab input
    const char *input = argv[1];
    size_t input_len = strlen(input);

    // parse utf8
    uint32_t *codepoints = malloc(input_len * sizeof(uint32_t));
    uint32_t *codepoints_pos = &codepoints[0];
    const char *input_pos = input;
    ret = passgen_utf8_decode(&codepoints_pos, input_len, NULL, (const uint8_t **) &input_pos, input_len);

    if(ret != 0) {
        printf("Error: UTF-8 decode returned %i: %s\n", ret, passgen_utf8_error(ret));
        return 1;
    }

    size_t codepoints_len = codepoints_pos - codepoints;
    printf("Read %zu codepoints\n", codepoints_len);

    struct passgen_token token;
    struct passgen_token_parser parser;
    passgen_token_parser_init(&parser);

    for(size_t i = 0; i < codepoints_len; i++) {
        ret = passgen_token_parse(&parser, &token, 1, codepoints[i]);
        if(ret != 0) {
            printf("Error: Token parser error %i: %s\n", ret, passgen_token_parse_error_str(ret));
        } else {
            printf("Parsed token: %x\n", token.codepoint);
        }
    }

    free(codepoints);
    return 0;
}

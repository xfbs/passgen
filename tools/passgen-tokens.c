#include <stdio.h>
#include <jansson.h>
#include "passgen/token.h"
#include "passgen/token_json.h"
#include "passgen/utf8.h"

int main(int argc, char *argv[]) {
    unsigned char stdin_buffer[128];
    uint32_t codepoint_buffer[128];
    size_t stdin_buffer_len;

    while((stdin_buffer_len = fread(stdin_buffer, 1, sizeof(stdin_buffer), stdin)) > 0) {
        if(ferror(stdin)) {
            break;
        }

        printf("%zu\n", stdin_buffer_len);
    }

    return EXIT_SUCCESS;
}

void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <string>\n", prog);
    fprintf(stderr, "Decodes the tokens and prints them out as JSON.\n");
}

void dump(const unsigned char *str) {
    struct token_parser parser = {0};
    struct token token = {0};

    json_t *output = json_array();

    for(size_t i = 0; str[i]; i++) {
        json_t *obj = json_object();

        json_object_set_new(obj, "character", json_integer(str[i]));

        int ret = token_parse(&parser, &token, str[i]);

        json_t *parser_json = token_parser_to_json(&parser);
        json_object_set_new(obj, "parser", parser_json);

        if(parser.state == TOKEN_INIT) {
            json_t *token_json = token_to_json(&token);
            json_object_set_new(obj, "token", token_json);
        }

        json_array_append_new(output, obj);
    }

    json_dumpf(output, stdout, JSON_INDENT(4));
    json_decref(output);
}

#include <assert.h>
#include <string.h>
#include "passgen/data/parser.h"
#include "passgen/data/token.h"
#include <passgen/passgen.h>
#include "passgen/parser.h"
#include "passgen/random.h"
#include "passgen/token.h"
#include "passgen/utf8.h"

int passgen_parse(
    struct passgen_pattern *output,
    passgen_error *error,
    const char *string) {
    struct passgen_parser parser;
    struct passgen_token_parser token_parser = {0};
    struct passgen_token token = {0};
    passgen_parser_init(&parser);

    size_t pattern_max = 256;
    uint32_t pattern[pattern_max];
    uint8_t pattern_widths[pattern_max];
    size_t pattern_pos = 0;
    size_t pattern_pos_total = 0;
    size_t pattern_raw_pos = 0;
    size_t pattern_raw_len = strlen(string);

    int decode_ret = 0;

    do {
        // decode input utf8 into unicode codepoints
        decode_ret = passgen_utf8_decode(
            pattern,
            pattern_max,
            &pattern_pos,
            pattern_widths,
            (const unsigned char *) string,
            pattern_raw_len,
            &pattern_raw_pos);

        pattern_pos_total += pattern_pos;

        if(decode_ret < 0) {
            error->codepoint = pattern_pos_total;
            error->byte = pattern_raw_pos;
            error->message = passgen_utf8_error(decode_ret);
            return -1;
        }

        // parse tokens and pattern
        for(size_t i = 0; i < pattern_pos; i++) {
            int ret = passgen_token_parse(
                    &token_parser,
                    &token,
                    pattern_widths[i],
                    pattern[i]);

            if(ret == PASSGEN_TOKEN_INIT) {
                ret = passgen_parse_token(&parser, &token);
                if(ret != 0) {
                    error->codepoint = token.offset;
                    error->byte = token.byte_offset;
                    error->message = "Error parsing";
                    return -1;
                }
                assert(ret == 0);
            }
        }
    } while(decode_ret > 0);

    // make sure we aren't still parsing tokens
    if(token_parser.state != PASSGEN_TOKEN_INIT) {
        error->codepoint = pattern_pos_total;
        error->byte = pattern_raw_pos;
        error->message = passgen_token_parse_error_str(token_parser.state);
        return -1;
    }

    // make sure we are done parsing
    if(0 != passgen_parse_finish(&parser)) {
        error->codepoint = pattern_pos_total;
        error->byte = pattern_raw_pos;
        error->message = "Parsing not finished";
    }

    *output = parser.pattern;

    return 0;
}

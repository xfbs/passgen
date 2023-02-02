#include "passgen/parser/parser.h"
#include "passgen/parser/token.h"
#include "passgen/pattern/pattern.h"
#include "passgen/util/random.h"
#include "passgen/util/utf8.h"
#include <assert.h>
#include <passgen/passgen.h>
#include <string.h>

int passgen_parse(
    passgen_pattern *output,
    passgen_error *error,
    const char *string) {
    passgen_parser parser;
    passgen_token_parser token_parser = {0};
    passgen_token token = {0};
    passgen_parser_init(&parser, output);

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
        pattern_pos = 0;
        decode_ret = passgen_utf8_decode(
            pattern,
            pattern_max,
            &pattern_pos,
            pattern_widths,
            (const unsigned char *) string,
            pattern_raw_len,
            &pattern_raw_pos);

        pattern_pos_total += pattern_pos;

        // make sure utf8 decoding was successful
        if(decode_ret < 0) {
            error->codepoint = pattern_pos_total;
            error->byte = pattern_raw_pos;
            error->message = passgen_utf8_error(decode_ret);
            passgen_parser_free(&parser);
            return -1;
        }

        // parse tokens and pattern
        for(size_t i = 0; i < pattern_pos; i++) {
            int ret = passgen_token_parse(
                &token_parser,
                &token,
                pattern_widths[i],
                pattern[i]);

            // make sure parsing the token was successful
            if(ret < 0) {
                error->codepoint = token.offset;
                error->byte = token.byte_offset;
                error->message = passgen_token_parse_error_str(ret);
                passgen_parser_free(&parser);
                return -1;
            }

            // if we have a token, feed it to the parser
            if(ret == PASSGEN_TOKEN_INIT) {
                ret = passgen_parse_token(&parser, &token);
                // make sure that the parser accepts the token
                if(ret != 0) {
                    error->codepoint = token.offset;
                    error->byte = token.byte_offset;
                    error->message = "invalid token";
                    passgen_parser_free(&parser);
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
        passgen_parser_free(&parser);
        return -1;
    }

    // make sure we are done parsing
    if(0 != passgen_parse_finish(&parser)) {
        error->codepoint = pattern_pos_total;
        error->byte = pattern_raw_pos;
        error->message = "parsing not finished";
        passgen_parser_free(&parser);
        return -1;
    }

    passgen_parser_free(&parser);

    return 0;
}

#include "passgen/parser/parser.h"
#include "passgen/parser/token.h"
#include "passgen/pattern/pattern.h"
#include "passgen/random.h"
#include "passgen/util/alloc.h"
#include "passgen/util/error.h"
#include "passgen/util/utf8.h"
#include <passgen/passgen.h>
#include <stdlib.h>
#include <string.h>

passgen_status passgen_parse(
    passgen_pattern **pattern,
    passgen_error *error,
    const char *string) {
    *pattern = passgen_alloc(sizeof(passgen_pattern));
    if(!*pattern) {
        return passgen_error_set(error, PASSGEN_ERROR_OUT_OF_MEMORY);
    }

    passgen_status status = passgen_pattern_parse(*pattern, error, string);
    if(status != PASSGEN_OK) {
        // whatever was parsed up to the error is of no use to anyone.
        passgen_pattern_free(*pattern);
        *pattern = NULL;
    }

    return status;
}

/// Turns an error state of the tokenizer into a status.
static passgen_status token_error_status(int state) {
    switch(state) {
        case PASSGEN_TOKEN_ERROR_UNICODE_RANGE:
            return PASSGEN_ERROR_UNICODE_RANGE;
        case PASSGEN_TOKEN_ERROR_UNICODE_START:
        case PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD:
        case PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN:
            return PASSGEN_ERROR_UNICODE_ESCAPE;
        default:
            return PASSGEN_ERROR_INTERNAL;
    }
}

passgen_status passgen_pattern_parse(
    passgen_pattern *output,
    passgen_error *error,
    const char *string) {
    passgen_parser parser;
    passgen_token_parser token_parser = {0};
    passgen_token token = {0};
    if(PASSGEN_OK != passgen_parser_init(&parser, output)) {
        passgen_parser_free(&parser);
        return passgen_error_set(error, PASSGEN_ERROR_OUT_OF_MEMORY);
    }

    const uint8_t *pattern_raw = (const uint8_t *) string;
    size_t pattern_len = 256;
    uint32_t pattern[pattern_len];
    uint8_t pattern_widths[pattern_len];
    size_t pattern_pos = 0;
    size_t pattern_raw_len = strlen(string);

    // how much of the input was decoded so far.
    passgen_error_offset decoded = {0, 0};

    int decode_ret = 0;

    do {
        uint32_t *pattern_cur = &pattern[0];
        const uint8_t *pattern_raw_cur = &pattern_raw[decoded.byte];

        // decode input utf8 into unicode codepoints
        decode_ret = passgen_utf8_decode(
            &pattern_cur,
            pattern_len,
            pattern_widths,
            &pattern_raw_cur,
            pattern_raw_len - decoded.byte);

        // where this batch of codepoints starts.
        passgen_error_offset batch = decoded;

        pattern_pos = pattern_cur - &pattern[0];
        decoded.byte += pattern_raw_cur - &pattern_raw[decoded.byte];
        decoded.codepoint += pattern_pos;

        // parse tokens and pattern. this is done before looking at whether
        // decoding failed, so that errors come up in the order in which they
        // are in the pattern.
        passgen_error_offset position = batch;
        for(size_t i = 0; i < pattern_pos; i++) {
            // the codepoint following this one
            passgen_error_offset next = {
                position.codepoint + 1,
                position.byte + pattern_widths[i],
            };

            int ret = passgen_token_parse(
                &token_parser,
                &token,
                pattern_widths[i],
                pattern[i]);

            // a token can be more than one codepoint (for escape sequences),
            // and starts where the tokenizer says it does.
            passgen_error_offset token_start = {
                token.offset,
                token.byte_offset,
            };

            // make sure parsing the token was successful
            if(ret < 0) {
                passgen_parser_free(&parser);
                return passgen_error_set_at(
                    error,
                    token_error_status(ret),
                    token_start,
                    next);
            }

            // if we have a token, feed it to the parser
            if(ret == PASSGEN_TOKEN_INIT) {
                passgen_status status = passgen_parse_token(&parser, &token);
                // make sure that the parser accepts the token
                if(status == PASSGEN_ERROR_OUT_OF_MEMORY) {
                    passgen_parser_free(&parser);
                    return passgen_error_set(error, status);
                }
                if(status != PASSGEN_OK) {
                    passgen_parser_free(&parser);
                    return passgen_error_set_at(
                        error,
                        status,
                        token_start,
                        next);
                }
            }

            position = next;
        }

        // make sure utf8 decoding was successful. the error is at what
        // follows the last codepoint which could be decoded.
        if(decode_ret < PASSGEN_UTF8_SUCCESS) {
            passgen_error_offset end = {decoded.codepoint + 1, decoded.byte + 1};
            passgen_parser_free(&parser);
            return passgen_error_set_at(
                error,
                PASSGEN_ERROR_UTF8,
                decoded,
                end);
        }
    } while(decode_ret > PASSGEN_UTF8_SUCCESS);

    // from here on, the error is that something is missing at the end.

    // make sure we aren't still parsing tokens
    if(token_parser.state != PASSGEN_TOKEN_INIT) {
        passgen_parser_free(&parser);
        return passgen_error_set_at(
            error,
            PASSGEN_ERROR_ESCAPE_UNFINISHED,
            decoded,
            decoded);
    }

    // make sure we are done parsing
    passgen_status status = passgen_parse_finish(&parser);
    passgen_parser_free(&parser);
    if(status != PASSGEN_OK) {
        return passgen_error_set_at(error, status, decoded, decoded);
    }

    return PASSGEN_OK;
}

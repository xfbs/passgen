#include "passgen/parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "tests.h"

test_result test_parser_simple(void) {
    struct parser parser = {0};
    struct passgen_token_parser token_parser = {0};
    struct passgen_token token = {0};

    assert(0 == parser_init(&parser));
    assert(passgen_token_parse(&token_parser, &token, 'a') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));

    return test_ok;
}

test_result test_parser_multi_groups(void) {
    struct parser parser = {0};
    struct passgen_token_parser token_parser = {0};
    struct passgen_token token = {0};

    assert(0 == parser_init(&parser));
    assert(passgen_token_parse(&token_parser, &token, 'a') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));
    assert(passgen_token_parse(&token_parser, &token, '|') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));
    assert(passgen_token_parse(&token_parser, &token, 'b') == PASSGEN_TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));

    return test_ok;
}

#include "passgen/parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "tests.h"

test_result test_parser_simple(void) {
    struct parser parser = {0};
    struct token_parser token_parser = {0};
    struct token token = {0};

    assert(0 == parser_init(&parser));
    assert(token_parse(&token_parser, &token, 'a') == TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));

    return test_ok;
}

test_result test_parser_multi_groups(void) {
    struct parser parser = {0};
    struct token_parser token_parser = {0};
    struct token token = {0};

    assert(0 == parser_init(&parser));
    assert(token_parse(&token_parser, &token, 'a') == TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));
    assert(token_parse(&token_parser, &token, '|') == TOKEN_INIT);
    assert(0 == parse_token(&parser, &token));
    //assert(token_parse(&token_parser, &token, 'b') == TOKEN_INIT);
    //assert(0 == parse_token(&parser, &token));

    return test_ok;
}

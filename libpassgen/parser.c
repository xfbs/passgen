#include "passgen/parser.h"

int parser_init(
        struct parser *parser) {
    // initialise parsing stack
    parser->state = passgen_array_init(sizeof(struct parser_state), NULL);

    // initialise group
    parser->parsed.group.segments = passgen_array_init(sizeof(pattern_segments_t), NULL);

    // set initial group
    struct parser_state *state = passgen_array_push(&parser->state, sizeof(struct parser_state), NULL);
    state->type = PARSER_OUTER;
    state->data = &parser->parsed.group;

    // set initial segment
    state = passgen_array_push(&parser->state, sizeof(struct parser_state), NULL);
    state->type = PARSER_SEGMENT;
    struct pattern_segments *segments = passgen_array_push(&parser->parsed.group.segments, sizeof(pattern_segments_t), NULL);
    state->data = segments;
    segments->items = passgen_array_init(sizeof(struct pattern_segment), NULL);

    return 0;
}

int parse_token_outer(
        struct parser *parser,
        struct token *token,
        struct pattern_group *group);

int parse_token_segment(
        struct parser *parser,
        struct token *token,
        struct pattern_segments *segments);

int parse_token(
        struct parser *parser,
        struct token *token) {

    int ret;

    do {
        // get current state
        printf("size %zu\n", parser->state.len);
        struct parser_state *state = passgen_array_get(
                &parser->state,
                sizeof(struct parser_state),
                parser->state.len - 1);

        printf("state: %p len %zu\n", state, parser->state.len);

        switch(state->type) {
            case PARSER_OUTER:
                ret = parse_token_outer(parser, token, state->data);
                break;
            case PARSER_SEGMENT:
                ret = parse_token_segment(parser, token, state->data);
                break;
        }
    } while(ret > 0);

    return ret;
}

int parse_token_outer(
        struct parser *parser,
        struct token *token,
        struct pattern_group *group) {
    if(token->codepoint == '|') {
        // create new segment and parser state
        struct parser_state *state = passgen_array_push(
                &parser->state,
                sizeof(struct parser_state),
                NULL);

        state->type = PARSER_SEGMENT;

        struct pattern_segments *segments = passgen_array_push(
                &parser->parsed.group.segments,
                sizeof(pattern_segments_t),
                NULL);

        state->data = segments;

        segments->items = passgen_array_init(sizeof(struct pattern_segment), NULL);

        return 0;
    } else {
        return -1;
    }
}

int parse_token_segment(
        struct parser *parser,
        struct token *token,
        struct pattern_segments *segments) {

    // check if this is a delimiter, and in that case, bubble up
    if(token->codepoint == '|') {
        passgen_array_pop(&parser->state, sizeof(struct parser_state), NULL);
        return 1;
    }

    struct pattern_segment *item = passgen_array_push(&segments->items, sizeof(struct pattern_segment), NULL);

    // we're supposed to read something in.
    if(token->codepoint == '(') {
        // start new group
    }

    if(token->codepoint == '[') {
        // start new ranges
    }

    item->kind = PATTERN_CHAR;
    item->data.character.codepoint = token->codepoint;

    return 0;
}

int parse_finish(
        struct parser *parser) {
    return 0;
}

#include "passgen/parser.h"
#include "passgen/pattern/segments.h"
#include "passgen/pattern/segment.h"
#include "passgen/pattern/kind.h"

static inline struct parser_state *parser_state_push(struct parser *parser) {
    return passgen_array_push(&parser->state, sizeof(struct parser_state), NULL);
}

static inline void parser_state_init(struct parser *parser) {
    passgen_array_init(&parser->state, sizeof(struct parser_state), NULL);
}

static inline void parser_state_pop(struct parser *parser) {
    passgen_array_pop(&parser->state, sizeof(struct parser_state), NULL);
}

int parser_init(
        struct parser *parser) {
    // initialise parsing stack
    parser_state_init(parser);

    // initialise group
    passgen_pattern_init(&parser->parsed);
    //passgen_array_init(&parser->parsed.group.segments, sizeof(passgen_pattern_segments_t), NULL);

    // set initial group
    struct parser_state *state = parser_state_push(parser);
    state->type = PARSER_GROUP;
    state->data = &parser->parsed.group;
    state->_data.group.group = &parser->parsed.group;
    //struct passgen_pattern_segments *segments = passgen_array_push(&parser->parsed.group.segments, sizeof(passgen_pattern_segments_t), NULL);
    state->_data.group.segments = passgen_pattern_group_add(&parser->parsed.group);
    //passgen_array_init(&segments->items, sizeof(struct passgen_pattern_segment), NULL);

    /*
    // set initial segment
    state = passgen_array_push(&parser->state, sizeof(struct parser_state), NULL);
    state->type = PARSER_SEGMENT;
    state->data = segments;
    passgen_array_init(&segments->items, sizeof(struct passgen_pattern_segment), NULL);
    */

    return 0;
}

int parse_token_group(
        struct parser *parser,
        struct token *token,
        struct parser_state *state);

int parse_token_segment(
        struct parser *parser,
        struct token *token,
        struct passgen_pattern_segments *segments);

int parse_token(
        struct parser *parser,
        struct token *token) {

    int ret;

    do {
        // get current state
        struct parser_state *state = passgen_array_get(
                &parser->state,
                sizeof(struct parser_state),
                parser->state.len - 1);

        switch(state->type) {
            case PARSER_GROUP:
                ret = parse_token_group(parser, token, state);
                break;
            case PARSER_SEGMENT:
                ret = parse_token_segment(parser, token, state->data);
                break;
        }
    } while(ret > 0);

    return ret;
}

int parse_token_group(
        struct parser *parser,
        struct token *token,
        struct parser_state *state) {
    if(token->codepoint == '|') {
        // create new segment and parser state
        struct passgen_pattern_segments *segments = passgen_array_push(
                &parser->parsed.group.segments,
                sizeof(passgen_pattern_segments_t),
                NULL);
        state->_data.group.segments = segments;
        passgen_array_init(&segments->items, sizeof(struct passgen_pattern_segment), NULL);

        return 0;
    }

    struct passgen_pattern_segment *item = passgen_array_push(&state->_data.group.segments->items, sizeof(struct passgen_pattern_segment), NULL);

    // we're supposed to read something in.
    if(token->codepoint == '(') {
        item = PASSGEN_PATTERN_GROUP;
        struct parser_state *state = parser_state_push(parser);
        state->type = PARSER_GROUP;
        state->data = &item->data.group;
    }

    if(token->codepoint == '[') {
        item = PASSGEN_PATTERN_RANGE;
        struct parser_state *state = parser_state_push(parser);
        state->type = PARSER_RANGE;
        state->data = &item->data.range;
    }

    item->kind = PASSGEN_PATTERN_CHAR;
    item->data.character.codepoint = token->codepoint;

    return 0;
}

int parse_token_segment(
        struct parser *parser,
        struct token *token,
        struct passgen_pattern_segments *segments) {

    // check if this is a delimiter, and in that case, bubble up
    if(token->codepoint == '|') {
        parser_state_pop(parser);
        return 1;
    }

    struct passgen_pattern_segment *item = passgen_array_push(&segments->items, sizeof(struct passgen_pattern_segment), NULL);

    // we're supposed to read something in.
    if(token->codepoint == '(') {
        item = PASSGEN_PATTERN_GROUP;
        struct parser_state *state = parser_state_push(parser);
        state->type = PARSER_GROUP;
        state->data = &item->data.group;
    }

    if(token->codepoint == '[') {
        item = PASSGEN_PATTERN_RANGE;
        struct parser_state *state = parser_state_push(parser);
        state->type = PARSER_RANGE;
        state->data = &item->data.range;
    }

    item->kind = PASSGEN_PATTERN_CHAR;
    item->data.character.codepoint = token->codepoint;

    return 0;
}

int parse_finish(
        struct parser *parser) {
    return 0;
}

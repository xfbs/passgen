#include "passgen/parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "passgen/container/stack/range.h"
#include "passgen/container/stack/segment_item.h"
#include "passgen/data/chars.h"
#include "passgen/data/group.h"
#include "passgen/data/parser.h"
#include "passgen/data/parser_state.h"
#include "passgen/data/pattern.h"
#include "passgen/data/range.h"
#include "passgen/data/repeat.h"
#include "passgen/data/segment.h"
#include "passgen/data/segment_item.h"
#include "passgen/data/set.h"
#include "passgen/data/token.h"
#include "passgen/memory.h"

int passgen_parse_start(struct passgen_parser *parser) {
    if(parser->state.len != 0) {
        return -1;
    }

    // set initial group
    passgen_parser_state_push_group(
        parser,
        &parser->pattern.group,
        passgen_pattern_group_new_segment(&parser->pattern.group));

    return 0;
}

// get the last item, making sure that it's only a single character.
// in case of characters, mark it as tainted.
static inline struct passgen_pattern_item *
last_single_item_taint(struct passgen_pattern_segment *segment) {
    struct passgen_pattern_item *item =
        passgen_pattern_item_stack_top(&segment->items);

    if(item->kind == PASSGEN_PATTERN_CHAR) {
        if(item->data.character.count > 1) {
            // save last codepoint
            int32_t codepoint =
                item->data.character.codepoints[item->data.character.count - 1];

            // trim codepoints
            item->data.character.count -= 1;

            // create new item
            item = passgen_pattern_segment_new_item(segment);
            item->kind = PASSGEN_PATTERN_CHAR;
            item->data.character.count = 1;
            item->data.character.codepoints[0] = codepoint;
        }

        // characters are always marked as tainted.
        item->data.character.tainted = true;
    }

    return item;
}

int passgen_parse_token(
    struct passgen_parser *parser,
    struct passgen_token *token) {
    struct passgen_parser_state *state = passgen_parser_get_state_last(parser);

    switch(state->type) {
        case PASSGEN_PARSER_GROUP:
            return passgen_parse_group(parser, token, state);
        case PASSGEN_PARSER_SET:
            return passgen_parse_set(parser, token, state);
        case PASSGEN_PARSER_SET_RANGE:
            return passgen_parse_set_range(parser, token, state);
        case PASSGEN_PARSER_REPEAT:
            return passgen_parse_repeat(parser, token, state);
        case PASSGEN_PARSER_REPEAT_RANGE:
            return passgen_parse_repeat_range(parser, token, state);
        case PASSGEN_PARSER_SPECIAL:
            return passgen_parse_special(parser, token, state);
        case PASSGEN_PARSER_SPECIAL_NAME:
            return passgen_parse_special_name(parser, token, state);
        case PASSGEN_PARSER_SPECIAL_NAME_END:
            return passgen_parse_special_name_end(parser, token, state);
        default:
            return -1;
    }
}

int passgen_parse_group(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
    uint32_t codepoint = token->codepoint;
    struct passgen_pattern_group *group;
    struct passgen_pattern_special *special;
    struct passgen_pattern_item *item;

    if(codepoint & PASSGEN_TOKEN_ESCAPED_BIT) {
        codepoint &= ~PASSGEN_TOKEN_ESCAPED_BIT;
        switch((char) codepoint) {
            case '|':
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
                // escaped token which would normally do something but
                // should be treated as text
                break;
            case 'p':
            case 'w':
                // special token
                special = passgen_pattern_segment_new_special(
                    state->data.group.segment);
                passgen_pattern_special_init_char(special, (char) codepoint);
                passgen_parser_state_push_special(parser, special);
                return 0;
            default:
                // error
                return -1;
        }
    } else {
        switch((char) codepoint) {
            case '|':
                // create new segment and parser state
                state->data.group.segment =
                    passgen_pattern_group_new_segment(state->data.group.group);
                return 0;
            case ')':
                passgen_parser_state_stack_pop(&parser->state, NULL);
                return 0;
            case '(':
                // we're supposed to read something in.
                group = passgen_pattern_segment_new_group(
                    state->data.group.segment);
                passgen_parser_state_push_group(
                    parser,
                    group,
                    passgen_pattern_group_new_segment(group));
                return 0;
            case '[':
                passgen_parser_state_push_set(
                    parser,
                    passgen_pattern_segment_new_set(state->data.group.segment),
                    NULL);
                return 0;
            case '{':
                item = last_single_item_taint(state->data.group.segment);
                passgen_parser_state_push_repeat(parser, &item->repeat);
                return 0;
            case '?':
                item = last_single_item_taint(state->data.group.segment);
                item->maybe = true;
                return 0;
            default:
                break;
        }
    }

    // check if the last item was a character that we can add this one to
    if(state->data.group.segment->items.len) {
        struct passgen_pattern_item *last =
            passgen_pattern_item_stack_top(&state->data.group.segment->items);

        if(last->kind == PASSGEN_PATTERN_CHAR) {
            if(last->data.character.count < 7 &&
               !last->data.character.tainted) {
                last->data.character.codepoints[last->data.character.count] =
                    codepoint;
                last->data.character.count += 1;

                return 0;
            }
        }
    }

    struct passgen_chars *chr =
        passgen_pattern_segment_new_char(state->data.group.segment);
    chr->count = 1;
    chr->tainted = 0;
    chr->codepoints[0] = codepoint;

    return 0;
}

int passgen_parse_set(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
    struct passgen_pattern_set *set = state->data.set.set;

    // this set's over
    if(token->codepoint == ']') {
        // compute sum of choices and choices list for binary search.
        size_t choices = 0;
        set->choices_list =
            passgen_malloc(NULL, sizeof(size_t) * set->items.len);
        for(size_t i = 0; i < set->items.len; i++) {
            struct passgen_pattern_range *range =
                passgen_pattern_range_stack_get(&set->items, i);
            choices += 1 + range->end - range->start;
            set->choices_list[i] = choices;
        }

        passgen_parser_state_stack_pop(&parser->state, NULL);
        return 0;
    }

    // part of a range expression
    if(token->codepoint == '-') {
        state->type = PASSGEN_PARSER_SET_RANGE;
        return 0;
    }

    struct passgen_pattern_range *range = passgen_pattern_set_new_range(set);

    range->start = token->codepoint;
    range->end = token->codepoint;

    state->data.set.range = range;

    return 0;
}

int passgen_parse_set_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
    (void) parser;
    state->data.set.range->end = token->codepoint;
    state->type = PASSGEN_PARSER_SET;

    return 0;
}

int passgen_parse_repeat(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
    // this set's over
    if(token->codepoint == '}') {
        state->data.repeat.repeat->max = state->data.repeat.repeat->min;
        passgen_parser_state_stack_pop(&parser->state, NULL);
        return 0;
    }

    if(token->codepoint == ',') {
        state->data.repeat.repeat->max = 0;
        state->type = PASSGEN_PARSER_REPEAT_RANGE;
        return 0;
    }

    if(token->codepoint >= '0' && token->codepoint <= '9') {
        uint8_t digit = token->codepoint - '0';

        state->data.repeat.repeat->min *= 10;
        state->data.repeat.repeat->min += digit;

        return 0;
    }

    return -1;
}

int passgen_parse_repeat_range(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
    if(token->codepoint == '}') {
        passgen_parser_state_stack_pop(&parser->state, NULL);
        return 0;
    }

    if(token->codepoint >= '0' && token->codepoint <= '9') {
        uint8_t digit = token->codepoint - '0';

        state->data.repeat.repeat->max *= 10;
        state->data.repeat.repeat->max += digit;

        return 0;
    }

    return -1;
}

int passgen_parse_special(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
    (void) parser;

    if(token->codepoint == '[') {
        state->type = PASSGEN_PARSER_SPECIAL_NAME;
        return 0;
    }

    return -1;
}

int passgen_parse_special_name(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
    (void) parser;

    if(token->codepoint == ']') {
        state->type = PASSGEN_PARSER_SPECIAL_NAME_END;
    }

    return 0;
}

int passgen_parse_special_name_end(
    struct passgen_parser *parser,
    struct passgen_token *token,
    struct passgen_parser_state *state) {
    if(token->codepoint == '{') {
        struct passgen_pattern_repeat *length =
            &state->data.special.special->length;
        length->min = 0;
        length->max = 0;
        passgen_parser_state_stack_pop(&parser->state, NULL);
        passgen_parser_state_push_repeat(parser, length);
        return 0;
    }

    return -1;
}

int passgen_parse_finish(struct passgen_parser *parser) {
    // make sure we just have one state on the stack, the initial one.
    if(parser->state.len != 1) {
        return -1;
    }

    return 0;
}

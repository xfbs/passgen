#include "passgen/pattern.h"
#include "passgen/pattern_private.h"
#include <assert.h>

const char *passgen_error_str[PATTERN_ERROR_LAST] = {
    "cannot allocate memory.",
    "illegal.",
    "depth limit exceeded.",
    "group close token expected here.",
};

static const pattern_result_t result_ok = {
    .ok = true,
};

pattern_result_t pattern_error_illegal(passgen_token_t *token) {
    return (pattern_result_t) {
        .ok = false,
        .kind = PATTERN_ERROR_ILLEGAL,
        .pos.offset = token->pos.offset,
        .pos.length = token->pos.length,
    };
}

pattern_result_t pattern_error_depth(passgen_token_t *token) {
    return (pattern_result_t) {
        .ok = false,
        .remove = true,
        .kind = PATTERN_ERROR_DEPTH,
        .pos.offset = token->pos.offset,
        .pos.length = token->pos.length,
    };
}

pattern_result_t pattern_error_token(passgen_token_t *token, enum pattern_error kind) {
    return (pattern_result_t) {
        .ok = false,
        .remove = true,
        .kind = kind,
        .pos.offset = token->pos.offset,
        .pos.length = token->pos.length,
    };
}

pattern_result_t pattern_error_alloc(unicode_iter_t *iter) {
    return (pattern_result_t) {
        .ok = false,
        .kind = PATTERN_ERROR_ILLEGAL,
        .pos.offset = iter->pos,
        .pos.length = 1,
    };
}

static pattern_result_t
pattern_parse_repeat(
        pattern_repeat_t *repeat,
        unicode_iter_t *iter);

void pattern_group_free(pattern_group_t *group, passgen_mem_t *mem);

void pattern_range_free(pattern_ranges_t *ranges, passgen_mem_t *mem) {
    passgen_array_free(&ranges->items, sizeof(pattern_range_t), mem);
    passgen_free(mem, ranges->choices_list);
}

void pattern_segment_free(pattern_segment_t *segment, passgen_mem_t *mem) {
    switch(segment->kind) {
        case PATTERN_RANGE:
            pattern_range_free(&segment->data.range, mem);
            break;
        case PATTERN_SPECIAL:
            break;
        case PATTERN_GROUP:
            pattern_group_free(&segment->data.group, mem);
            break;
        case PATTERN_CHAR:
            break;
    }
}

void pattern_segments_free(pattern_segments_t *segments, passgen_mem_t *mem) {
    for(size_t i = 0; i < segments->items.len; i++) {
        pattern_segment_t *segment = passgen_array_get(
                &segments->items,
                sizeof(pattern_segment_t),
                i);

        pattern_segment_free(segment, mem);
    }

    passgen_array_free(&segments->items, sizeof(pattern_segment_t), mem);
}

void pattern_group_free(pattern_group_t *group, passgen_mem_t *mem) {
    // go through segments, free individually
    for(size_t i = 0; i < group->segments.len; i++) {
        pattern_segments_t *segments = passgen_array_get(
                &group->segments,
                sizeof(pattern_segments_t),
                i);

        pattern_segments_free(segments, mem);
    }

    // free array holding them
    passgen_array_free(&group->segments, sizeof(pattern_segments_t), mem);
}

void pattern_free(pattern_t *pattern) {
    pattern_group_free(&pattern->group, pattern->mem);
}

pattern_result_t pattern_segments_parse(
        pattern_segments_t *segment,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem)
{
    // save the current position.
    segment->pos.offset = iter->pos;
    segment->pos.length = 0;

    segment->items = passgen_array_init(sizeof(pattern_segment_t), mem);
    bool first = true;

    while(!pattern_segments_is_end(passgen_token_peek(iter))) {
        pattern_segment_t *item = passgen_array_push(
                &segment->items,
                sizeof(pattern_segment_t),
                mem);
        if(!item) {
            return pattern_error_alloc(iter);
        }

        pattern_result_t result = pattern_segment_parse(item, iter, depth, mem);
        if(!result.ok) {
            return result;
        }

        // update length
        segment->pos.length = iter->pos - segment->pos.offset;
        first = false;
    }

    return result_ok;
}

pattern_result_t pattern_group_parse(
        pattern_group_t *group,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem)
{
    passgen_token_t token;
    pattern_result_t result;

    token = passgen_token_next(iter);

    /* if this was called, this is true already. */
    if(!pattern_group_is_start(token)) {
        // error
    }

    result = pattern_group_parse_inner(
            group,
            iter,
            depth,
            mem);

    if(!result.ok) {
        return result;
    }

    token = passgen_token_next(iter);

    if(!pattern_group_is_end(token)) {
        return pattern_error_token(&token, PATTERN_ERROR_GROUP_CLOSE);
    }

    result = pattern_parse_repeat(
            &group->repeat,
            iter);

    if(!result.ok) {
        return result;
    }

    return result_ok;
}

pattern_result_t pattern_segment_parse(
        pattern_segment_t *item,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem)
{
    pattern_result_t result;

    passgen_token_t token = passgen_token_peek(iter);

    if(pattern_group_is_start(token)) {
        if(depth == 0) {
            return pattern_error_depth(&token);
        } else {
            item->kind = PATTERN_GROUP;
            result = pattern_group_parse(&item->data.group, iter, depth - 1, mem);
        }
    } else if(pattern_ranges_is_start(token)) {
        item->kind = PATTERN_RANGE;
        result = pattern_ranges_parse(&item->data.range, iter, mem);
    } else if(passgen_token_is_special(&token)) {
        item->kind = PATTERN_SPECIAL;
        result = pattern_special_parse(&item->data.special, iter, mem);
    } else {
        item->kind = PATTERN_CHAR;
        result = pattern_char_parse(&item->data.character, iter, mem);
    }

    return result;
}

pattern_result_t pattern_char_parse(
        pattern_char_t *character,
        unicode_iter_t *iter,
        passgen_mem_t *mem)
{
    pattern_result_t result;
    character->pos.offset = iter->pos;

    passgen_token_t token = passgen_token_next(iter);
    character->codepoint = token.codepoint;

    result = pattern_parse_repeat(
            &character->repeat,
            iter);

    character->pos.length = iter->pos - character->pos.offset;

    return result_ok;
}

pattern_result_t pattern_range_parse(
        pattern_range_t *range,
        unicode_iter_t *iter,
        passgen_mem_t *mem)
{
    passgen_token_t token;

    range->pos.offset = iter->pos;

    token = passgen_token_next(iter);

    range->start = token.codepoint;

    token = passgen_token_peek(iter);

    if(pattern_range_is_sep(token)) {
        passgen_token_next(iter);

        token = passgen_token_next(iter);
        range->end = token.codepoint;
    } else {
        range->end = range->start;
    }

    range->pos.length = iter->pos - range->pos.offset;

    return result_ok;
}

pattern_result_t pattern_ranges_parse_inner(
        pattern_ranges_t *range,
        unicode_iter_t *iter,
        passgen_mem_t *mem)
{
    passgen_token_t token;
    pattern_result_t result;

    range->repeat.min = 1;
    range->repeat.max = 1;

    // initialise array of range items
    range->items = passgen_array_init(sizeof(pattern_range_t), mem);

    // parse range items until we hit end of range
    token = passgen_token_peek(iter);
    while(!pattern_ranges_is_end(token)) {
        pattern_range_t *range_item = passgen_array_push(
                &range->items,
                sizeof(pattern_range_t),
                mem);

        result = pattern_range_parse(range_item, iter, mem);

        if(!result.ok) {
            return result;
        }

        token = passgen_token_peek(iter);
    }

    return result_ok;
}

pattern_result_t pattern_ranges_parse(
        pattern_ranges_t *ranges,
        unicode_iter_t *iter,
        passgen_mem_t *mem)
{
    ranges->pos.offset = iter->pos;
    passgen_token_t token = passgen_token_next(iter);
    // TODO assert this?

    pattern_result_t result;
    result = pattern_ranges_parse_inner(ranges, iter, mem);

    token = passgen_token_next(iter);
    // assert this is ']'

    result = pattern_parse_repeat(
            &ranges->repeat,
            iter);

    if(!result.ok) {
        return result;
    }

    ranges->pos.length = iter->pos - ranges->pos.offset;

    // compute sum of choices and choices list for binary search.
    size_t choices = 0;
    ranges->choices_list = passgen_malloc(mem, sizeof(size_t) * ranges->items.len);
    for(size_t i = 0; i < ranges->items.len; i++) {
        pattern_range_t *range = passgen_array_get(&ranges->items, sizeof(pattern_range_t), i);
        choices += 1 + range->end - range->start;
        ranges->choices_list[i] = choices;
    }

    return result_ok;
}

/// Parse the content of a group.
pattern_result_t pattern_group_parse_inner(
        pattern_group_t *group,
        unicode_iter_t *iter,
        size_t depth,
        passgen_mem_t *mem)
{
    // save the current position.
    group->pos.offset = iter->pos;
    group->pos.length = 0;

    group->segments = passgen_array_init(sizeof(pattern_segments_t), mem);
    bool first = true;

    while(first || pattern_group_is_separator(passgen_token_peek(iter))) {
        // skip separator.
        if(!first) {
            passgen_token_next(iter);
        }

        pattern_segments_t *segment = passgen_array_push(
                &group->segments,
                sizeof(pattern_segments_t),
                mem);
        if(!segment) {
            return pattern_error_alloc(iter);
        }

        pattern_result_t result = pattern_segments_parse(segment, iter, depth, mem);
        if(!result.ok) {
            return result;
        }

        // update length
        group->pos.length = iter->pos - group->pos.offset;
        first = false;
    }

    return result_ok;
}

pattern_result_t
pattern_parse(
        pattern_t *pattern,
        const char *data,
        size_t depth,
        passgen_mem_t *mem)
{
    pattern->mem = mem;
    pattern->pattern = data;

    // parse segments and make sure everything went okay.
    unicode_iter_t iter = unicode_iter(data);
    pattern_result_t result = pattern_group_parse_inner(&pattern->group, &iter, depth, mem);
    if(!result.ok) {
        return result;
    }

    // force repetitions at one.
    pattern->group.repeat.min = 1;
    pattern->group.repeat.max = 1;

    // make sure we really have reached EOF.
    passgen_token_t token = passgen_token_peek(&iter);
    if(token.type != PATTERN_TOKEN_EOF) {
        return pattern_error_illegal(&token);
    }

    return result_ok;
}

pattern_result_t
pattern_special_parse(
        pattern_special_t *special,
        unicode_iter_t *iter,
        passgen_mem_t *mem)
{
    pattern_result_t result;

    special->pos.offset = iter->pos;

    /* skip first token — we know this is a special token. */
    passgen_token_t token = passgen_token_next(iter);

    result = pattern_parse_sqarg(
            &special->arg,
            iter);

    if(!result.ok) {
        return result;
    }

    switch(token.codepoint) {
        case 'p':
            special->kind = PATTERN_SPECIAL_PRONOUNCABLE;
            special->data.pronounceable = passgen_pronounceable_lookup(
                    special->arg.length,
                    iter->data + special->arg.offset);
            break;
        case 'w':
            special->kind = PATTERN_SPECIAL_WORDLIST;
            break;
        default:
            break;
    }

    /* parse length. */
    result = pattern_parse_repeat(
            &special->length,
            iter);

    if(!result.ok) {
        return result;
    }

    /* parse repetitions. */
    result = pattern_parse_repeat(
            &special->repeat,
            iter);

    if(!result.ok) {
        return result;
    }

    special->pos.length = iter->pos - special->pos.offset;

    return result_ok;
}

size_t pattern_maxlen(pattern_t *pattern) {
    return 0;
}

size_t pattern_minlen(pattern_t *pattern) {
    return 0;
}

size_t pattern_choices(pattern_t *pattern) {
    return 0;
}

bool pattern_group_is_separator(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == '|') {
        return true;
    }

    return false;
}

bool pattern_group_is_start(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == '(') {
        return true;
    }

    return false;
}

bool pattern_group_is_end(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == ')') {
        return true;
    }

    return false;
}

bool pattern_ranges_is_start(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == '[') {
        return true;
    }

    return false;
}

bool pattern_ranges_is_end(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == ']') {
        return true;
    }

    return false;
}

bool pattern_range_is_sep(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR && token.codepoint == '-') {
        return true;
    }

    return false;
}

bool pattern_segments_is_end(passgen_token_t token) {
    if(token.type == PATTERN_TOKEN_REGULAR) {
        switch(token.codepoint) {
            case ']':
            case ')':
            case '|':
                return true;
            default:
                break;
        }
    }

    // FIXME!
    /*
    if(token.type == PATTERN_TOKEN_ERROR) {
        return true;
    }
    */

    if(token.type == PATTERN_TOKEN_EOF) {
        return true;
    }

    return false;
}

bool pattern_repeat_is_start(passgen_token_t token) {
    return passgen_token_is_regular(&token) && token.codepoint == '{';
}

bool pattern_repeat_is_sep(passgen_token_t token) {
    return passgen_token_is_regular(&token) && token.codepoint == ',';
}

bool pattern_repeat_is_end(passgen_token_t token) {
    return passgen_token_is_regular(&token) && token.codepoint == '}';
}

bool pattern_sqarg_is_start(passgen_token_t token) {
    return passgen_token_is_regular(&token) && token.codepoint == '[';
}

bool pattern_sqarg_is_end(passgen_token_t token) {
    return passgen_token_is_regular(&token) && token.codepoint == ']';
}

static pattern_result_t
pattern_parse_repeat(
        pattern_repeat_t *repeat,
        unicode_iter_t *iter)
{
    unicode_iter_t prev = *iter;
    pattern_result_t result = result_ok;

    passgen_token_t token = passgen_token_next(iter);

    // don't do anything if this isn't a start token.
    if(!pattern_repeat_is_start(token)) {
        *iter = prev;
        repeat->min = 1;
        repeat->max = 1;
        return result;
    }

    // parse min
    result = passgen_parse_number(&repeat->min, iter);
    if(!result.ok) {
        return result;
    }

    token = passgen_token_peek(iter);

    if(pattern_repeat_is_sep(token)) {
        passgen_token_next(iter);
        // parse max
        result = passgen_parse_number(&repeat->max, iter);
        if(!result.ok) {
            return result;
        }
    } else {
        repeat->max = repeat->min;
    }

    // check if comma or end brace is next
    token = passgen_token_next(iter);

    if(!pattern_repeat_is_end(token)) {
        // error
    }

    if(repeat->min > repeat->max) {
        // error
    }

    return result;
}

static pattern_result_t
pattern_parse_sqarg(
        pattern_substring_t *arg,
        unicode_iter_t *iter)
{
    unicode_iter_t prev = *iter;
    passgen_token_t token = passgen_token_next(iter);

    // don't do anything if this isn't a start token.
    if(!pattern_sqarg_is_start(token)) {
        arg->offset = 0;
        arg->length = 0;
        *iter = prev;
        return result_ok;
    }

    arg->offset = iter->pos;

    while(!passgen_token_is_error(&token) && !pattern_sqarg_is_end(token)) {
        arg->length = iter->pos - arg->offset;
        token = passgen_token_next(iter);
    }

    if(passgen_token_is_error(&token)) {
        // error
    }

    return result_ok;
}

static inline pattern_result_t
passgen_parse_number(
        size_t *number,
        unicode_iter_t *iter)
{
    unicode_iter_t prev = *iter;
    passgen_token_t token;
    size_t num = 0;
    size_t len = 0;

    token = passgen_token_next(iter);
    while(true) {
        if(!passgen_token_is_regular(&token)) {
            break;
        }

        if('0' <= token.codepoint && token.codepoint <= '9') {
            num *= 10;
            num += token.codepoint - '0';
        } else {
            break;
        }

        len += 1;
        prev = *iter;
        token = passgen_token_next(iter);
    }

    if(len == 0) {
        // error
    }

    if(passgen_token_is_error(&token)) {
        // error
    }

    // that last thing we aren't interested in.
    *iter = prev;

    // store result
    *number = num;
    return result_ok;
}

const char *pattern_error_to_str(enum pattern_error kind) {
    return passgen_error_str[kind];
}

void pattern_error_show(pattern_result_t result, const char *format) {
    printf("\e[31mError at position %zu while parsing pattern: %s\n\e[0m", 
            result.pos.offset,
            pattern_error_to_str(result.kind));

    printf("\n%s\n", format);

    for(size_t i = 0; i < result.pos.offset; i++) {
        printf(" ");
    }

    for(size_t i = 0; i < (1 || result.pos.length); i++) {
        printf(i ? "~" : "^");
    }

    printf("\n");
}

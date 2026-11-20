/// @file output.c
///
/// Generates the sequences, and writes them out, one per line (or separated
/// by NUL characters) or as JSON.
#include "passgen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Growable output buffer. Generated sequences have no fixed upper bound on
/// their length, so they are collected in here.
typedef struct {
    char *data;
    size_t len;
    size_t capacity;
    bool json;
} passgen_cli_buffer;

static int passgen_cli_buffer_push(
    passgen_cli_buffer *buffer,
    const char *data,
    size_t len) {
    if(buffer->capacity - buffer->len < len) {
        size_t capacity = buffer->capacity ? buffer->capacity : 256;
        while(capacity - buffer->len < len) {
            capacity *= 2;
        }
        char *new_data = realloc(buffer->data, capacity);
        if(!new_data) {
            return -1;
        }
        buffer->data = new_data;
        buffer->capacity = capacity;
    }

    memcpy(&buffer->data[buffer->len], data, len);
    buffer->len += len;
    return 0;
}

/// Encodes a codepoint as UTF-8, and returns how many bytes that took, or
/// zero if it is not a codepoint which can be encoded.
static int passgen_cli_utf8_encode(uint8_t *out, uint32_t codepoint) {
    if(codepoint <= 0x7F) {
        out[0] = (uint8_t) codepoint;
        return 1;
    }
    if(codepoint <= 0x7FF) {
        out[0] = (uint8_t) (0xC0 | (codepoint >> 6));
        out[1] = (uint8_t) (0x80 | (codepoint & 0x3F));
        return 2;
    }
    if(codepoint <= 0xFFFF) {
        out[0] = (uint8_t) (0xE0 | (codepoint >> 12));
        out[1] = (uint8_t) (0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = (uint8_t) (0x80 | (codepoint & 0x3F));
        return 3;
    }
    if(codepoint <= 0x10FFFF) {
        out[0] = (uint8_t) (0xF0 | (codepoint >> 18));
        out[1] = (uint8_t) (0x80 | ((codepoint >> 12) & 0x3F));
        out[2] = (uint8_t) (0x80 | ((codepoint >> 6) & 0x3F));
        out[3] = (uint8_t) (0x80 | (codepoint & 0x3F));
        return 4;
    }
    return 0;
}

static int
passgen_cli_buffer_puts(passgen_cli_buffer *buffer, const char *str) {
    return passgen_cli_buffer_push(buffer, str, strlen(str));
}

/// Callback for passgen_generate(): appends a codepoint as UTF-8 to the
/// buffer, escaping it for use in a JSON string if requested.
static int passgen_cli_buffer_emit(void *data, uint32_t codepoint) {
    passgen_cli_buffer *buffer = data;
    char encoded[8];

    if(buffer->json) {
        switch(codepoint) {
            case '"':
                return passgen_cli_buffer_puts(buffer, "\\\"");
            case '\\':
                return passgen_cli_buffer_puts(buffer, "\\\\");
            case '\b':
                return passgen_cli_buffer_puts(buffer, "\\b");
            case '\f':
                return passgen_cli_buffer_puts(buffer, "\\f");
            case '\n':
                return passgen_cli_buffer_puts(buffer, "\\n");
            case '\r':
                return passgen_cli_buffer_puts(buffer, "\\r");
            case '\t':
                return passgen_cli_buffer_puts(buffer, "\\t");
            default:
                break;
        }

        if(codepoint < 0x20) {
            snprintf(encoded, sizeof(encoded), "\\u%04x", (unsigned) codepoint);
            return passgen_cli_buffer_puts(buffer, encoded);
        }
    }

    int bytes = passgen_cli_utf8_encode((uint8_t *) encoded, codepoint);
    if(bytes <= 0) {
        return -1;
    }

    return passgen_cli_buffer_push(buffer, encoded, bytes);
}

/// Reports why generating failed, and returns what to exit with.
static int
passgen_cli_generate_error(passgen_status status, passgen_error *error) {
    // the only reason for the callback to stop generating is that the buffer
    // for the output could not be made larger.
    if(status == PASSGEN_ERROR_ABORTED) {
        passgen_error_free(error);
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }

    passgen_cli_error_print("%s", "");
    passgen_error_print(error, stderr);
    passgen_error_free(error);

    return PASSGEN_CLI_ERROR_GENERATE;
}

// Switches the source of randomness to the stream for the sequence with the
// index, so that what each of them is does not depend on how much randomness
// the ones before it used. Sources without streams (the system, a file) are
// read from one after another, unless an offset was given, which is an error.
static int passgen_cli_stream(passgen_cli_opts *opts, size_t index) {
    if(!passgen_random_stream(opts->random, opts->offset + index) &&
       opts->offset_given) {
        passgen_cli_error_print(
            "--offset needs a deterministic source "
            "of randomness, such as --master\n");
        return PASSGEN_CLI_ERROR_ILLEGAL_OFFSET;
    }
    return 0;
}

static int
passgen_cli_generate_normal(passgen_cli_opts opts, passgen_pattern *pattern) {
    passgen_cli_buffer buffer = {0};
    char sep = opts.null ? '\0' : '\n';

    for(size_t i = 0; i < opts.amount; ++i) {
        int ret = passgen_cli_stream(&opts, i);
        if(ret != 0) {
            free(buffer.data);
            return ret;
        }

        double entropy;
        passgen_error error;
        buffer.len = 0;
        passgen_status status = passgen_generate(
            pattern,
            opts.env,
            &error,
            opts.entropy ? &entropy : NULL,
            &buffer,
            passgen_cli_buffer_emit);
        if(status != PASSGEN_OK) {
            free(buffer.data);
            return passgen_cli_generate_error(status, &error);
        }

        if(opts.entropy) {
            fprintf(stderr, "entropy: %.2lf bits\n", entropy);
        }

        // the separator (or the newline after the last one) follows right
        // away, and it is written out before the entropy of the next one goes
        // to standard error, which is not buffered. otherwise they would come
        // out of order where both go to the same place, such as a terminal.
        fwrite(buffer.data, 1, buffer.len, stdout);
        fputc(i + 1 < opts.amount ? sep : '\n', stdout);
        fflush(stdout);
    }

    free(buffer.data);

    return 0;
}

static int
passgen_cli_generate_json(passgen_cli_opts opts, passgen_pattern *pattern) {
    // the whole document is collected first, so that nothing is printed (rather
    // than truncated JSON) when generating fails.
    passgen_cli_buffer buffer = {.json = true};

    // this is nonzero when the buffer could not be made larger.
    int ret = passgen_cli_buffer_puts(&buffer, "[");

    for(size_t i = 0; ret == 0 && i < opts.amount; ++i) {
        int stream = passgen_cli_stream(&opts, i);
        if(stream != 0) {
            free(buffer.data);
            return stream;
        }

        double entropy;
        ret = passgen_cli_buffer_puts(
            &buffer,
            (i == 0) ? "{\"output\":\"" : ",{\"output\":\"");
        if(ret != 0) {
            break;
        }

        passgen_error error;
        passgen_status status = passgen_generate(
            pattern,
            opts.env,
            &error,
            opts.entropy ? &entropy : NULL,
            &buffer,
            passgen_cli_buffer_emit);
        if(status != PASSGEN_OK) {
            free(buffer.data);
            return passgen_cli_generate_error(status, &error);
        }

        if(opts.entropy) {
            char number[64];
            snprintf(number, sizeof(number), "\",\"entropy\":%lf}", entropy);
            ret = passgen_cli_buffer_puts(&buffer, number);
        } else {
            ret = passgen_cli_buffer_puts(&buffer, "\"}");
        }
    }

    if(ret == 0) {
        ret = passgen_cli_buffer_puts(&buffer, "]\n");
    }

    if(ret != 0) {
        free(buffer.data);
        return passgen_cli_bail(PASSGEN_CLI_ERROR_ALLOC, NULL);
    }

    fwrite(buffer.data, 1, buffer.len, stdout);
    free(buffer.data);
    return 0;
}

int passgen_cli_generate(passgen_cli_opts opts, passgen_pattern *pattern) {
    if(!opts.json) {
        return passgen_cli_generate_normal(opts, pattern);
    } else {
        return passgen_cli_generate_json(opts, pattern);
    }
}

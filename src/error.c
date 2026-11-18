#include "passgen/util/alloc.h"
#include "passgen/util/error.h"
#include <passgen/config.h>
#include <passgen/error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *passgen_status_string(passgen_status status) {
    switch(status) {
        case PASSGEN_OK:
            return "success";
        case PASSGEN_ERROR_OUT_OF_MEMORY:
            return "out of memory";
        case PASSGEN_ERROR_INTERNAL:
            return "internal error";
        case PASSGEN_ERROR_INVALID_ARGUMENT:
            return "invalid argument";
        case PASSGEN_ERROR_IO:
            return "cannot read file";
        case PASSGEN_ERROR_UTF8:
            return "not valid UTF-8";
        case PASSGEN_ERROR_ESCAPE:
            return "unknown escape sequence";
        case PASSGEN_ERROR_ESCAPE_UNFINISHED:
            return "unfinished escape sequence";
        case PASSGEN_ERROR_UNICODE_ESCAPE:
            return "malformed unicode escape";
        case PASSGEN_ERROR_UNICODE_RANGE:
            return "unicode escape is not a valid codepoint";
        case PASSGEN_ERROR_GROUP_UNOPENED:
            return "closing parenthesis without an opening one";
        case PASSGEN_ERROR_GROUP_UNCLOSED:
            return "group is not closed";
        case PASSGEN_ERROR_SET_UNCLOSED:
            return "set is not closed";
        case PASSGEN_ERROR_SET_EMPTY:
            return "set is empty";
        case PASSGEN_ERROR_SET_RANGE:
            return "invalid range in set";
        case PASSGEN_ERROR_REPEAT:
            return "invalid repetition";
        case PASSGEN_ERROR_OPTIONAL:
            return "nothing to make optional";
        case PASSGEN_ERROR_SPECIAL:
            return "expected a name in braces";
        case PASSGEN_ERROR_DEPTH_LIMIT:
            return "nested too deeply";
        case PASSGEN_ERROR_WORDLIST_NOT_FOUND:
            return "wordlist not found";
        case PASSGEN_ERROR_WORDLIST_EMPTY:
            return "wordlist is empty";
        case PASSGEN_ERROR_WORDLIST_READ:
            return "wordlist could not be read";
        case PASSGEN_ERROR_GROUP_EMPTY:
            return "nothing to choose from";
        case PASSGEN_ERROR_RANDOM:
            return "source of randomness ran out";
        case PASSGEN_ERROR_BUFFER_TOO_SMALL:
            return "output does not fit into the buffer";
        case PASSGEN_ERROR_ABORTED:
            return "stopped";
    }

    // not a status.
    return "unknown error";
}

void passgen_error_init(passgen_error *error) {
    memset(error, 0, sizeof(*error));
    error->kind = PASSGEN_OK;
}

void passgen_error_free(passgen_error *error) {
    if(!error) {
        return;
    }

    // the causes are a list, each of which was allocated.
    passgen_error *cause = error->cause;
    while(cause) {
        passgen_error *next = cause->cause;
        passgen_free(cause, sizeof(passgen_error));
        cause = next;
    }

    passgen_error_init(error);
}

void passgen_error_print(const passgen_error *error, FILE *file) {
    for(const passgen_error *current = error; current;
        current = current->cause) {
        if(current != error) {
            fprintf(file, "caused by: ");
        }

        fprintf(file, "%s", passgen_status_string(current->kind));
        if(current->detail[0]) {
            fprintf(file, ": %s", current->detail);
        }
        fprintf(file, "\n");
    }
}

passgen_status passgen_error_set(passgen_error *error, passgen_status kind) {
    if(error) {
        passgen_error_init(error);
        error->kind = kind;
    }

    return kind;
}

passgen_status passgen_error_set_at(
    passgen_error *error,
    passgen_status kind,
    passgen_error_offset start,
    passgen_error_offset end) {
    if(error) {
        passgen_error_init(error);
        error->kind = kind;
        error->has_position = true;
        error->start = start;
        error->end = end;
    }

    return kind;
}

void passgen_error_detail_set(passgen_error *error, const char *detail) {
    if(!error) {
        return;
    }

    memset(error->detail, 0, sizeof(error->detail));
    if(detail) {
        strncpy(error->detail, detail, sizeof(error->detail) - 1);
    }
}

passgen_status passgen_error_wrap(
    passgen_error *error,
    passgen_status kind,
    passgen_error *cause) {
    // the cause may be the error itself, which is the common case of adding
    // context to an error on the way up. so move it out of the way first.
    passgen_error *moved = NULL;
    if(cause && cause->kind != PASSGEN_OK) {
        if(error) {
            moved = passgen_alloc(sizeof(passgen_error));
        }

        if(moved) {
            // this takes over the causes of the cause.
            *moved = *cause;
            passgen_error_init(cause);
        } else {
            // without anywhere to keep it, the cause is lost.
            passgen_error_free(cause);
        }
    }

    if(error) {
        passgen_error_init(error);
        error->kind = kind;
        error->cause = moved;
    }

    return kind;
}

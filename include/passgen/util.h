#pragma once

#include <stdint.h>

static inline int8_t hex_decode(uint32_t c) {
    if('0' <= c && c <= '9') {
        return c - '0';
    }

    if('A' <= c && c <= 'F') {
        return 10 + c - 'A';
    }

    if('a' <= c && c <= 'f') {
        return 10 + c - 'a';
    }

    return -1;
}

#pragma once

#define try(statement)       \
    do {                     \
        int ret = statement; \
        if(ret != 0) {       \
            return ret;      \
        }                    \
    } while(0)

#pragma once
#include <stddef.h>

size_t passgen_pronounceable(char *buf, size_t len);

int passgen_pronounceable_len(char *buf, size_t len, size_t tries);

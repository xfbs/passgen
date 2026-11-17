/// @file clear.h
/// @brief Wiping memory which may have held secrets.
#pragma once
#include <passgen/config.h>

#ifdef PASSGEN_MONOCYPHER
#include <passgen/monocypher.h>

/// Wipes the value that a pointer points to, in a way which the compiler does
/// not optimize away.
#define PASSGEN_CLEAR(value) passgen_wipe((value), sizeof(*(value)))
#else
#include <string.h>

/// Zeroes the value that a pointer points to. Without Monocypher, there is
/// nothing which keeps the compiler from optimizing this away.
#define PASSGEN_CLEAR(value) memset((value), 0, sizeof(*(value)))
#endif

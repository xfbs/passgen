/// @file stats.h
///
/// Checking that random choices are made with the right probabilities.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Whether how often each outcome came up fits how likely it is meant to be,
/// using Pearson's chi-squared test.
///
/// It fails if counts which are this far off would only come up by chance
/// with a probability of less than one in a million. The tests use fixed
/// seeds, so whether it passes does not change from run to run. Bias, such
/// as some outcomes being twice as likely as others, fails it by far.
///
/// @param outcomes How many outcomes there are, at least two
/// @param counts How often each outcome came up
/// @param weights How likely each outcome is meant to be, relative to the
///   others (they do not have to add up to one)
/// @returns Whether the counts fit the weights
bool chi_squared_fits(
    size_t outcomes,
    const uint64_t *counts,
    const double *weights);

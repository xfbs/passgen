#include "stats.h"
#include <math.h>
#include <stdio.h>

// the standard normal quantile for a probability of one in a million (one
// sided).
#define Z_ONE_IN_A_MILLION 4.753424

bool chi_squared_fits(
    size_t outcomes,
    const uint64_t *counts,
    const double *weights) {
    uint64_t total = 0;
    double weight_sum = 0;
    for(size_t i = 0; i < outcomes; i++) {
        total += counts[i];
        weight_sum += weights[i];
    }

    double statistic = 0;
    for(size_t i = 0; i < outcomes; i++) {
        double expected = (double) total * weights[i] / weight_sum;
        if(expected == 0) {
            // something which cannot happen did.
            if(counts[i] != 0) {
                return false;
            }
            continue;
        }
        double difference = (double) counts[i] - expected;
        statistic += difference * difference / expected;
    }

    // the critical value of the chi-squared distribution, using the
    // approximation of Wilson and Hilferty, which is close enough for this.
    double df = (double) (outcomes - 1);
    double term = 1 - 2 / (9 * df) + Z_ONE_IN_A_MILLION * sqrt(2 / (9 * df));
    double critical = df * term * term * term;

    if(statistic > critical) {
        fprintf(
            stderr,
            "chi-squared of %.1f is over %.1f (%zu outcomes, %llu samples)\n",
            statistic,
            critical,
            outcomes,
            (unsigned long long) total);
        return false;
    }

    return true;
}

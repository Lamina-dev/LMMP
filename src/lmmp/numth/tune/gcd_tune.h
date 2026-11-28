/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  This library is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef LMMP_NUMTH_TUNE_GCD_H
#define LMMP_NUMTH_TUNE_GCD_H

#include <math.h>

#include "support/perf.h"
#include "tune/common.h"

enum { GCD_TUNE_MAX_ROUNDS = 9 };

typedef struct {
    double low_us, high_us, low_mad, high_mad;
    unsigned retries;
    bool stable;
} gcd_tune_pair;

static bool gcd_tune_parse_rounds(void* opaque, const char* option, const char* value) {
    char* end;
    unsigned long rounds = strtoul(value, &end, 10);
    if (strcmp(option, "--rounds") != 0 || end == value || *end != '\0' ||
        rounds > GCD_TUNE_MAX_ROUNDS || (rounds != 0 && (rounds & 1) == 0)) {
        fprintf(stderr, "expected --rounds with 0 or an odd integer in [1,%d]\n", GCD_TUNE_MAX_ROUNDS);
        return false;
    }
    *(unsigned*)opaque = (unsigned)rounds;
    return true;
}

static void gcd_tune_noop(void* opaque) { (void)opaque; }

/* 用同一对采样块的耗时比抵消频率漂移。0 表示该点缺少稳定读数。 */
static bool gcd_tune_measure(mp_limb_t (*call)(void*), void* low_context, void* high_context,
                            const lmmp_tune_options* options, gcd_tune_pair* pair, double* ratio) {
    lmmp_tune_bench low = {.bind = gcd_tune_noop, .reset = gcd_tune_noop, .call = call, .context = low_context};
    lmmp_tune_bench high = low;
    high.context = high_context;
    lmmp_tune_pair_trace trace;
    if (!lmmp_tune_measure_pair_traced(&low, &high, options, &pair->low_us, &pair->low_mad, &pair->high_us,
                                      &pair->high_mad, &pair->retries, &pair->stable, &trace))
        return false;
    *ratio = 0;
    if (!pair->stable)
        return true;
    double samples[ARRAY_SIZE(trace.low_us)];
    for (unsigned i = 0; i < options->samples; ++i) {
        if (trace.low_us[i] <= 0 || trace.high_us[i] <= 0)
            return false;
        samples[i] = trace.high_us[i] / trace.low_us[i];
    }
    *ratio = bench_aggregate(samples, options->samples, BENCH_MEDIAN);
    return isfinite(*ratio) && *ratio > 0;
}

static double gcd_tune_median(const double* ratios, unsigned rounds) {
    double stable[GCD_TUNE_MAX_ROUNDS];
    unsigned count = 0;
    for (unsigned i = 0; i < rounds; ++i)
        if (ratios[i] > 0)
            stable[count++] = ratios[i];
    return count > rounds / 2 ? bench_aggregate(stable, count, BENCH_MEDIAN) : 0;
}

#endif

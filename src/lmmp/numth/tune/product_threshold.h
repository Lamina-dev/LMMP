/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
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

#ifndef LMMP_NUMTH_TUNE_PRODUCT_THRESHOLD_H
#define LMMP_NUMTH_TUNE_PRODUCT_THRESHOLD_H

#include "tune/common.h"

enum { PRODUCT_TUNE_MIN = 4, PRODUCT_TUNE_MAX = 161 };

typedef struct {
    lmmp_tune_bench baseline, candidate;
    mp_size_t current;
    mp_size_t* threshold;
    bool (*check)(void* context, mp_srcptr expected, mp_size_t expected_n);
} product_tune_pair;

typedef struct {
    const char* shape;
    mp_size_t n, rn;
    mp_srcptr expected;
    mp_size_t expected_n;
    const bool* starts;
} product_tune_input;

typedef struct {
    double scores[PRODUCT_TUNE_MAX + 1];
    unsigned stable, unstable;
} product_tune_scores;

/* 每个候选区间内的递归分派相同。逐区间对现值配对，同一输入要么给所有候选计分，要么全部剔除。 */
static bool measure_product_input(const product_tune_pair* pair,
                                  const product_tune_input* input,
                                  int max_threshold,
                                  const lmmp_tune_options* options,
                                  product_tune_scores* result) {
    if (!pair->check(pair->baseline.context, input->expected, input->expected_n)) {
        fprintf(stderr, "%s baseline mismatch at n=%llu\n", input->shape, (unsigned long long)input->n);
        return false;
    }

    bool row_stable = true;
    double ratios[PRODUCT_TUNE_MAX + 1];
    for (int first = PRODUCT_TUNE_MIN, last; first <= max_threshold; first = last + 1) {
        for (last = first; last < max_threshold && !input->starts[last + 1]; ++last) {
        }
        bool same_path = (mp_size_t)first <= pair->current && pair->current <= (mp_size_t)last;
        double ratio = 1;
        if (!same_path || input->n % 8 == 0) {
            *pair->threshold = same_path ? pair->current : (mp_size_t)first;
            if (!pair->check(pair->candidate.context, input->expected, input->expected_n)) {
                fprintf(stderr, "%s mismatch at n=%llu threshold=%llu\n", input->shape, (unsigned long long)input->n,
                        (unsigned long long)*pair->threshold);
                return false;
            }
            double base_us, base_mad, us, mad;
            unsigned retries;
            bool stable;
            if (!lmmp_tune_measure_pair(&pair->baseline, &pair->candidate, options, &base_us, &base_mad, &us, &mad,
                                        &retries, &stable) ||
                base_us <= 0 || us <= 0) {
                fprintf(stderr, "%s invalid timing at n=%llu threshold=%llu\n", input->shape,
                        (unsigned long long)input->n, (unsigned long long)*pair->threshold);
                return false;
            }
            double measured_ratio = us / base_us;
            printf("%s,%llu,%llu,%d,%d,%d,%.6f,%.6f,%.6f,%.4f,%.4f,%u,%d\n", input->shape, (unsigned long long)input->n,
                   (unsigned long long)input->rn, first, last, same_path, base_us, us, measured_ratio, base_mad, mad,
                   retries, stable);
            if (!same_path) {
                ratio = measured_ratio;
                row_stable &= stable;
            }
        }
        for (int t = first; t <= last; ++t) ratios[t] = ratio;
    }
    if (row_stable) {
        for (int t = PRODUCT_TUNE_MIN; t <= max_threshold; ++t) result->scores[t] += ratios[t];
        ++result->stable;
    } else {
        ++result->unstable;
    }
    return true;
}

static void print_product_scores(const char* name,
                                 const char* shape,
                                 const product_tune_scores* result,
                                 int max_threshold,
                                 mp_size_t current) {
    printf("# shape=%s stable_inputs=%u unstable_inputs=%u\n", shape, result->stable, result->unstable);
    if (result->stable == 0) {
        puts("# no stable inputs; recommendation withheld");
        return;
    }
    int best = (int)current;
    for (int t = PRODUCT_TUNE_MIN; t <= max_threshold; ++t) {
        printf("# candidate=%d mean_ratio=%.6f\n", t, result->scores[t] / result->stable);
        if (result->scores[t] < result->scores[best])
            best = t;
    }
    double limit = result->scores[best] * 1.01;
    int low = best, high = best;
    while (low > PRODUCT_TUNE_MIN && result->scores[low - 1] <= limit) --low;
    while (high < max_threshold && result->scores[high + 1] <= limit) ++high;
    printf("# flat_within_1pct=%d..%d\n", low, high);
    printf("recommended %s = %d\n", name, best);
}

#endif

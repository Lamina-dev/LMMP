/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
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

#ifndef LMMP_PERF_H
#define LMMP_PERF_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lmmp/lmmp.h"
#include "lmmp/mprand.h"
#include "lmmp/numth.h"

#if defined(LMMP_WINDOWS)
#include <windows.h>
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef enum { BENCH_MIN, BENCH_MEDIAN, BENCH_MEAN } bench_aggregation;

typedef struct {
    bench_aggregation agg;
    unsigned max_samples;
    double min_block_sec;
    double slow_block_sec;
    mp_size_t max_iters;
} bench_options;

static const bench_options bench_default = {BENCH_MIN, 5, 0.02, 0.05, 1ull << 26};
static volatile mp_limb_t bench_sink;

static inline double bench_clock(void) {
#if defined(LMMP_WINDOWS)
    static double ticks_to_seconds;
    LARGE_INTEGER ticks;
    if (ticks_to_seconds == 0) {
        QueryPerformanceFrequency(&ticks);
        ticks_to_seconds = 1.0 / (double)ticks.QuadPart;
    }
    QueryPerformanceCounter(&ticks);
    return (double)ticks.QuadPart * ticks_to_seconds;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + 1e-9 * (double)now.tv_nsec;
#endif
}

static inline int bench_compare_times(const void *lhs, const void *rhs) {
    double a = *(const double *)lhs, b = *(const double *)rhs;
    return (a > b) - (a < b);
}

static inline double bench_aggregate(double *samples, unsigned n, bench_aggregation agg) {
    if (agg == BENCH_MEDIAN) {
        qsort(samples, n, sizeof(*samples), bench_compare_times);
        return (samples[(n - 1) / 2] + samples[n / 2]) * 0.5;
    }
    double result = samples[0];
    for (unsigned i = 1; i < n; ++i) {
        if (agg == BENCH_MEAN)
            result += samples[i];
        else if (samples[i] < result)
            result = samples[i];
    }
    return agg == BENCH_MEAN ? result / n : result;
}

// 调用表达式在计时循环内直接展开。
#define BENCH_MEASURE(result, options, ...)                                                             \
    do {                                                                                                \
        const bench_options *bench_opts_ = (options);                                                   \
        { __VA_ARGS__ }                                                                                 \
        mp_size_t bench_iters_ = 1;                                                                     \
        double bench_elapsed_;                                                                          \
        for (;;) {                                                                                      \
            double bench_start_ = bench_clock();                                                        \
            for (mp_size_t bench_i_ = 0; bench_i_ < bench_iters_; ++bench_i_) { __VA_ARGS__ }           \
            bench_elapsed_ = bench_clock() - bench_start_;                                              \
            if (bench_elapsed_ >= bench_opts_->min_block_sec || bench_iters_ >= bench_opts_->max_iters) \
                break;                                                                                  \
            bench_iters_ = LMMP_MIN(2 * bench_iters_, bench_opts_->max_iters);                          \
        }                                                                                               \
        unsigned bench_count_ = bench_opts_->max_samples ? bench_opts_->max_samples : 1;                \
        if (bench_elapsed_ >= bench_opts_->slow_block_sec && bench_count_ > 2)                          \
            bench_count_ = 2;                                                                           \
        double bench_samples_[bench_count_];                                                            \
        for (unsigned bench_s_ = 0; bench_s_ < bench_count_; ++bench_s_) {                              \
            double bench_start_ = bench_clock();                                                        \
            for (mp_size_t bench_i_ = 0; bench_i_ < bench_iters_; ++bench_i_) { __VA_ARGS__ }           \
            bench_samples_[bench_s_] = (bench_clock() - bench_start_) * 1e6 / bench_iters_;             \
        }                                                                                               \
        (result) = bench_aggregate(bench_samples_, bench_count_, bench_opts_->agg);                     \
    } while (0)

static inline mp_ptr alloc_limbs(mp_size_t n) {
    return lmmp_alloc(n * sizeof(mp_limb_t));
}

static inline void fill_random(mp_ptr p, mp_size_t n, mp_limb_t seed) {
    lmmp_seed_random_(p, n, seed, 1);
    if (n != 0)
        p[n - 1] |= (mp_limb_t)1 << 63;
}

static inline void report(const char *name, double us, double bytes) {
    double ops = us > 0 ? 1e6 / us : 0;
    printf("  %-40s %14.4f us/op  %14.2f ops/s", name, us, ops);
    if (bytes > 0)
        printf("  %10.2f MiB/s", bytes * ops / (1024.0 * 1024.0));
    putchar('\n');
}

static inline void report_n(const char *name, mp_size_t n, double us, double bytes) {
    char label[96];
    snprintf(label, sizeof(label), "%s(%lluL)", name, (unsigned long long)n);
    report(label, us, bytes);
}

static inline void report_ab(const char *name, mp_size_t a, mp_size_t b, double us, double bytes) {
    char label[96];
    snprintf(label, sizeof(label), "%s(%llux%llu)", name, (unsigned long long)a, (unsigned long long)b);
    report(label, us, bytes);
}

static inline void report_bits(const char *name, uint bits, double us) {
    char label[96];
    snprintf(label, sizeof(label), "%s(%ub)", name, bits);
    report(label, us, 0);
}

#endif

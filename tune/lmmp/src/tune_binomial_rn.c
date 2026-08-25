/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/* 阈值调优：BINOMIAL_RN_BASECASE_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

typedef struct {
    uint n;
    uint r;
    int is_uint;
    mp_size_t rn;
    mp_ptr dst;
} ncr_pair_t;

static double bench_ncr_low(void* v) {
    ncr_pair_t* c = (ncr_pair_t*)v;
    lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD = (uint64_t)c->rn + 1;
    if (c->is_uint)
        (void)lmmp_odd_nCr_uint_(c->dst, c->rn, c->n, c->r);
    else
        (void)lmmp_odd_nCr_ushort_(c->dst, c->rn, c->n, c->r);
    return 0.0;
}

static double bench_ncr_high(void* v) {
    ncr_pair_t* c = (ncr_pair_t*)v;
    lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD = 4;
    if (c->is_uint)
        (void)lmmp_odd_nCr_uint_(c->dst, c->rn, c->n, c->r);
    else
        (void)lmmp_odd_nCr_ushort_(c->dst, c->rn, c->n, c->r);
    return 0.0;
}

static int add_pair(ncr_pair_t* pairs, size_t* npoints, size_t cap,
                    uint n, uint r, int is_uint) {
    mp_bitcnt_t bits = 0;
    if (r < 25 || r > n / 2)
        return 0;
    const mp_size_t rn = lmmp_nCr_size_(n, r, &bits);
    if (rn < 8 || rn > 320)
        return 0;
    for (size_t i = 0; i < *npoints; ++i)
        if (pairs[i].rn == rn)
            return 0;
    if (*npoints >= cap)
        return 0;
    pairs[*npoints].n = n;
    pairs[*npoints].r = r;
    pairs[*npoints].is_uint = is_uint;
    pairs[*npoints].rn = rn;
    pairs[*npoints].dst = NULL;
    ++(*npoints);
    return 1;
}

int tune_run_binomial_rn(void) {
    enum { MAXP = 256 };
    ncr_pair_t pairs[MAXP];
    tune_point_t points[MAXP];
    size_t npoints = 0;
    const uint64_t old_value = lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD;

    static const uint ushort_n[] = {200, 500, 1000, 2000, 5000,
                                    10000, 20000, 40000, 65535};
    static const uint ushort_rp[] = {2, 3, 5, 8, 10, 15, 20, 30, 40, 50};
    for (size_t i = 0; i < sizeof(ushort_n) / sizeof(ushort_n[0]); ++i)
        for (size_t j = 0; j < sizeof(ushort_rp) / sizeof(ushort_rp[0]); ++j)
            (void)add_pair(pairs, &npoints, MAXP,
                           ushort_n[i], ushort_n[i] * ushort_rp[j] / 100, 0);

    static const uint uint_n[] = {65536, 100000, 200000, 500000, 1000000};
    static const uint uint_rp[] = {1, 2, 5, 10, 20, 35};
    for (size_t i = 0; i < sizeof(uint_n) / sizeof(uint_n[0]); ++i)
        for (size_t j = 0; j < sizeof(uint_rp) / sizeof(uint_rp[0]); ++j)
            (void)add_pair(pairs, &npoints, MAXP,
                           uint_n[i], uint_n[i] * uint_rp[j] / 100, 1);

    for (size_t i = 0; i < npoints; ++i)
        for (size_t j = i + 1; j < npoints; ++j)
            if (pairs[j].rn < pairs[i].rn) {
                const ncr_pair_t tmp = pairs[i];
                pairs[i] = pairs[j];
                pairs[j] = tmp;
            }

    printf("  %llu representative rn sample points\n", (unsigned long long)npoints);
    for (size_t i = 0; i < npoints; ++i) {
        tune_measure_t mlow, mhigh;
        pairs[i].dst = (mp_ptr)lmmp_alloc((size_t)(pairs[i].rn + 2) * sizeof(mp_limb_t));
        tune_measure_pair(bench_ncr_low, &pairs[i], bench_ncr_high, &pairs[i],
                          g_tune.samples, g_tune.target_ms, &mlow, &mhigh);
        points[i].size = (uint64_t)pairs[i].rn;
        points[i].low_ns = mlow.median_ns;
        points[i].high_ns = mhigh.median_ns;
        points[i].low_mad = mlow.mad_ns;
        points[i].high_mad = mhigh.mad_ns;
        printf("    rn=%-4u n=%-8u r=%-8u %s basecase=%10.3f ns factor=%10.3f ns\n",
               (unsigned)pairs[i].rn, pairs[i].n, pairs[i].r,
               pairs[i].is_uint ? "uint" : "ushort",
               points[i].low_ns, points[i].high_ns);
        lmmp_free(pairs[i].dst);
        pairs[i].dst = NULL;
    }

    const tune_choice_t choice = tune_choose_1d(points, npoints, 8, 320,
                                                old_value, TUNE_HIGH_WHEN_GE);
    const double old_badness = tune_badness_at(points, npoints, old_value,
                                               TUNE_HIGH_WHEN_GE);
    const double new_badness = tune_badness_at(points, npoints, choice.threshold,
                                               TUNE_HIGH_WHEN_GE);
    lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD = choice.threshold;

    printf("  -> old=%llu new=%llu badness=%.6f plateau=[%llu,%llu]\n",
           (unsigned long long)old_value, (unsigned long long)choice.threshold,
           new_badness, (unsigned long long)choice.plateau_lo,
           (unsigned long long)choice.plateau_hi);
    tune_record_add("BINOMIAL_RN_BASECASE_THRESHOLD", old_value, choice.threshold,
                    old_badness, new_badness);
    return 0;
}

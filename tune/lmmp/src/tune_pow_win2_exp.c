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

/* 阈值调优：POW_WIN2_EXP_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

#define TUNE_POW_WIN2_FIXED_N 512

typedef struct {
    mp_ptr base;
    mp_ptr dst;
    mp_size_t rn;
    mp_size_t n;
    ulong exp;
} powwin_ctx;

static int win2_eligible(ulong exp) {
    return (exp % 4 == 3) ||
           (2u * (unsigned)lmmp_limb_popcnt_(exp) >= (unsigned)lmmp_limb_bits_(exp));
}

static void powwin_ctx_init(powwin_ctx* c, ulong exp) {
    c->n = TUNE_POW_WIN2_FIXED_N;
    c->base = (mp_ptr)lmmp_alloc((size_t)c->n * sizeof(mp_limb_t));
    tune_fill_limbs(c->base, c->n, UINT64_C(0x8f1bbcdcbfa97e39));
    c->base[0] |= 1u;
    c->base[c->n - 1] |= LIMB_B_2;
    c->exp = exp;
    c->rn = lmmp_pow_size_(c->base, c->n, exp);
    c->dst = (mp_ptr)lmmp_alloc((size_t)(c->rn + 2) * sizeof(mp_limb_t));
}

static void powwin_ctx_free(powwin_ctx* c) {
    lmmp_free(c->base);
    lmmp_free(c->dst);
}

static void apply_exp(uint64_t exp, int use_high) {
    lmmp_tune_POW_WIN2_N_THRESHOLD = TUNE_POW_WIN2_FIXED_N;
    lmmp_tune_POW_WIN2_EXP_THRESHOLD =
        use_high ? (exp > 0 ? exp - 1 : 0) : exp;
}

static double bench_powwin_low(void* v) {
    powwin_ctx* c = (powwin_ctx*)v;
    apply_exp(c->exp, 0);
    (void)lmmp_pow_(c->dst, c->rn, c->base, c->n, c->exp);
    return 0.0;
}

static double bench_powwin_high(void* v) {
    powwin_ctx* c = (powwin_ctx*)v;
    apply_exp(c->exp, 1);
    (void)lmmp_pow_(c->dst, c->rn, c->base, c->n, c->exp);
    return 0.0;
}

int tune_run_pow_win2_exp(void) {
    enum { MAXP = 256 };
    tune_point_t points[MAXP];
    size_t npoints = 0;
    const uint64_t old_value = lmmp_tune_POW_WIN2_EXP_THRESHOLD;
    const uint64_t saved_n = lmmp_tune_POW_WIN2_N_THRESHOLD;

    printf("  measuring only win2-eligible odd exponents...\n");
    for (uint64_t exp = 3; exp <= 255 && npoints < MAXP; ++exp) {
        if (!win2_eligible(exp))
            continue;
        powwin_ctx ctx;
        tune_measure_t mlow, mhigh;
        powwin_ctx_init(&ctx, (ulong)exp);
        tune_measure_pair(bench_powwin_low, &ctx, bench_powwin_high, &ctx,
                          g_tune.samples, g_tune.target_ms, &mlow, &mhigh);
        points[npoints].size = exp;
        points[npoints].low_ns = mlow.median_ns;
        points[npoints].high_ns = mhigh.median_ns;
        points[npoints].low_mad = mlow.mad_ns;
        points[npoints].high_mad = mhigh.mad_ns;
        printf("    exp=%-4llu pow_non_win2=%12.3f ns pow_win2=%12.3f ns\n",
               (unsigned long long)exp, mlow.median_ns, mhigh.median_ns);
        ++npoints;
        powwin_ctx_free(&ctx);
    }

    const tune_choice_t choice = tune_choose_1d(points, npoints, 1, 128,
                                                old_value, TUNE_HIGH_WHEN_GT);
    const double old_badness = tune_badness_at(points, npoints, old_value,
                                               TUNE_HIGH_WHEN_GT);
    const double new_badness = tune_badness_at(points, npoints, choice.threshold,
                                               TUNE_HIGH_WHEN_GT);
    lmmp_tune_POW_WIN2_EXP_THRESHOLD = choice.threshold;

    printf("  -> POW_WIN2_EXP_THRESHOLD old=%llu new=%llu badness=%.6f plateau=[%llu,%llu]\n",
           (unsigned long long)old_value, (unsigned long long)choice.threshold,
           new_badness, (unsigned long long)choice.plateau_lo,
           (unsigned long long)choice.plateau_hi);
    tune_record_add("POW_WIN2_EXP_THRESHOLD", old_value, choice.threshold,
                    old_badness, new_badness);
    lmmp_tune_POW_WIN2_N_THRESHOLD = saved_n;
    return 0;
}

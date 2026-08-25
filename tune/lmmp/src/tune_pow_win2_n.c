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

/* 阈值调优：POW_WIN2_N_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_POW_WIN2_N_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_POW_WIN2_N_THRESHOLD = v; }

#define TUNE_POW_WIN2_FIXED_EXP 63

typedef struct {
    mp_ptr base;
    mp_ptr dst;
    mp_size_t rn;
    mp_size_t n;
    ulong exp;
} powwin_ctx;

static void powwin_ctx_init(powwin_ctx* c, mp_size_t n) {
    c->n = n;
    c->base = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    tune_fill_limbs(c->base, n, UINT64_C(0xd1310ba698dfb5ac));
    c->base[0] |= 1u;
    c->base[n - 1] |= LIMB_B_2;
    c->exp = TUNE_POW_WIN2_FIXED_EXP;
    c->rn = lmmp_pow_size_(c->base, n, c->exp);
    c->dst = (mp_ptr)lmmp_alloc((size_t)(c->rn + 2) * sizeof(mp_limb_t));
}

static double bench_powwin(void* v) {
    powwin_ctx* c = (powwin_ctx*)v;
    (void)lmmp_pow_(c->dst, c->rn, c->base, c->n, c->exp);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    powwin_ctx* c = (powwin_ctx*)lmmp_alloc(sizeof(powwin_ctx));
    if (c != NULL)
        powwin_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    powwin_ctx* c = (powwin_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->base);
        lmmp_free(c->dst);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    lmmp_tune_POW_WIN2_EXP_THRESHOLD = TUNE_POW_WIN2_FIXED_EXP - 1;
    set_threshold(use_high ? (size > 0 ? size - 1 : 0) : size);
}

int tune_run_pow_win2_n(void) {
    tune_1d_spec_t spec;
    const uint64_t saved_exp = lmmp_tune_POW_WIN2_EXP_THRESHOLD;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "POW_WIN2_N_THRESHOLD";
    spec.low_name = "pow_basecase_odd";
    spec.high_name = "pow_win2";
    spec.lo = 10;
    spec.hi = 1536;
    spec.sample_lo = 16;
    spec.sample_hi = 1536;
    spec.pred = TUNE_HIGH_WHEN_GT;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_powwin;
    {
        const int rc = tune_run_1d(&spec);
        lmmp_tune_POW_WIN2_EXP_THRESHOLD = saved_exp;
        return rc;
    }
}

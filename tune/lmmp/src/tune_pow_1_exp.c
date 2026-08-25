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

/* 阈值调优：POW_1_EXP_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_POW_1_EXP_THRESHOLD; }
static void set_threshold(uint64_t v) { lmmp_tune_POW_1_EXP_THRESHOLD = v; }

typedef struct {
    mp_ptr base;
    mp_ptr dst;
    mp_size_t rn;
    ulong exp;
} pow1_ctx;

static void pow1_ctx_init(pow1_ctx* c, ulong exp) {
    c->base = (mp_ptr)lmmp_alloc(sizeof(mp_limb_t));
    c->base[0] = 3;
    c->exp = exp;
    c->rn = lmmp_pow_size_(c->base, 1, exp);
    c->dst = (mp_ptr)lmmp_alloc((size_t)(c->rn + 2) * sizeof(mp_limb_t));
}

static double bench_pow1(void* v) {
    pow1_ctx* c = (pow1_ctx*)v;
    (void)lmmp_pow_(c->dst, c->rn, c->base, 1, c->exp);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    pow1_ctx* c = (pow1_ctx*)lmmp_alloc(sizeof(pow1_ctx));
    if (c != NULL)
        pow1_ctx_init(c, (ulong)size);
    return c;
}

static void free_ctx(void* v) {
    pow1_ctx* c = (pow1_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->base);
        lmmp_free(c->dst);
        lmmp_free(c);
    }
}

/* 实际条件是 exp <= T 走连乘，exp > T 走 pow_1。 */
static void apply_path(uint64_t size, int use_high) {
    set_threshold(use_high ? (size > 0 ? size - 1 : 0) : size);
}

int tune_run_pow_1_exp(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "POW_1_EXP_THRESHOLD";
    spec.low_name = "pow_mul_1_loop";
    spec.high_name = "pow_1_window";
    spec.lo = 0;
    spec.hi = 128;
    spec.sample_lo = 1;
    spec.sample_hi = 128;
    spec.pred = TUNE_HIGH_WHEN_GT;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_pow1;
    return tune_run_1d(&spec);
}

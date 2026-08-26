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

/* 阈值调优：FACTORS_MUL_N_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp/impl/ele_mul.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_FACTORS_MUL_N_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_FACTORS_MUL_N_THRESHOLD = v; }

typedef struct {
    mp_ptr dst;
    mp_size_t rn;
    fac_t* fac;
    uint nfactors;
} factors_ctx;

static uint next_prime(uint v) {
    for (;;) {
        uint q;
        v += 2;
        for (q = 3; (uint64_t)q * q <= v; q += 2)
            if (v % q == 0)
                break;
        if ((uint64_t)q * q > v)
            return v;
    }
}

static void factors_ctx_init(factors_ctx* c, uint nfactors) {
    c->nfactors = nfactors;
    // 按每个质数的幂次为2个两个limb来计算得到的长度
    c->rn = 2 * nfactors + 4;
    c->dst = (mp_ptr)lmmp_alloc((size_t)(c->rn) * sizeof(mp_limb_t));
    c->fac = (fac_t*)lmmp_alloc((size_t)nfactors * sizeof(fac_t));
    c->fac[0].j = 1;
    c->fac[0].f = 1;
    uint p = 2;
    for (uint i = 1; i < nfactors; ++i) {
        p = next_prime(p);
        c->fac[i].f = p;
        c->fac[i].j = nfactors / (p - 1);
        if (c->fac[i].j == 0)
            c->fac[i].j = 1;
    }
}

static double bench_factors(void* v) {
    factors_ctx* c = (factors_ctx*)v;
    (void)lmmp_factors_mul_(c->dst, c->rn, c->fac, c->nfactors);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    factors_ctx* c = (factors_ctx*)lmmp_alloc(sizeof(factors_ctx));
    if (c != NULL)
        factors_ctx_init(c, (uint)size);
    return c;
}

static void free_ctx(void* v) {
    factors_ctx* c = (factors_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->dst);
        lmmp_free(c->fac);
        lmmp_free(c);
    }
}

/* 实际条件是 nfactors <= T 走连乘。 */
static void apply_path(uint64_t size, int use_high) {
    set_threshold(use_high ? (size > 0 ? size - 1 : 0) : size);
}

int tune_run_factors_mul_n(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "FACTORS_MUL_N_THRESHOLD";
    spec.low_name = "factors_sequential";
    spec.high_name = "factors_divide";
    spec.lo = 4;
    spec.hi = 64;
    spec.pred = TUNE_HIGH_WHEN_GT;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_factors;
    return tune_run_1d(&spec);
}

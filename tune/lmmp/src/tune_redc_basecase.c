/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/* 阈值调优：REDC_BASECASE_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_REDC_BASECASE_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_REDC_BASECASE_THRESHOLD = v; }

typedef struct {
    mp_ptr dp;
    mp_ptr bp;
    mp_ptr ep;
    mp_ptr mp;
    mp_size_t en;
    mp_size_t n;
} powmod_ctx;

static void powmod_ctx_init(powmod_ctx* c, mp_size_t n) {
    c->n = n;
    c->en = 2;
    c->dp = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->bp = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->ep = (mp_ptr)lmmp_alloc((size_t)c->en * sizeof(mp_limb_t));
    c->mp = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    tune_fill_limbs(c->mp, n, UINT64_C(0x3d6f1a92c8e70b45));
    c->mp[0] |= 1;
    c->mp[n - 1] |= (mp_limb_t)1 << 63;
    tune_fill_limbs(c->bp, n, UINT64_C(0x7b2e59d014a6c3f8));
    lmmp_div_(NULL, c->bp, c->bp, n, c->mp, n); /* 保证 b < m */
    if (lmmp_zero_q_(c->bp, n))
        c->bp[0] = 1;
    tune_fill_limbs(c->ep, c->en, UINT64_C(0x29c4f0e6d5318a77));
    c->ep[c->en - 1] |= (mp_limb_t)1 << 63;
}

/*
 * powmod_odd 内部经 REDC_BASECASE_THRESHOLD 在每步归约上选择 addmul_1
 * 链式 basecase 或 mullo(q)+高半积路径。以完整函数为黑盒测量，与真实
 * 调用路径完全一致；指数取 2 limb，使梯子归约步主导总耗时。寻优上界
 * 留在 REDC_MERSENNE_THRESHOLD(309) 之下，避免梅森层稀释交叉信号。
 */
static double bench_powmod(void* v) {
    powmod_ctx* c = (powmod_ctx*)v;
    lmmp_powmod_odd_(c->dp, c->bp, c->ep, c->en, c->mp, c->n);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    powmod_ctx* c = (powmod_ctx*)lmmp_alloc(sizeof(powmod_ctx));
    if (c != NULL)
        powmod_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    powmod_ctx* c = (powmod_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->dp);
        lmmp_free(c->bp);
        lmmp_free(c->ep);
        lmmp_free(c->mp);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    (void)size;
    set_threshold(use_high ? size : (UINT64_C(1) << 20));
}

int tune_run_redc_basecase(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "REDC_BASECASE_THRESHOLD";
    spec.low_name = "redc_basecase";
    spec.high_name = "redc_mullo_muln";
    spec.lo = 4;
    spec.hi = 300;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_powmod;
    return tune_run_1d(&spec);
}

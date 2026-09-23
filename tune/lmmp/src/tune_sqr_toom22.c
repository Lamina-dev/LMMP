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

/* 阈值调优：SQR_TOOM22_THRESHOLD (sqr_hard -> sqr_toom2)
   与乘法同构：搜索下限 20 受硬编码平方 LMMP_MUL_HARD_MAX_N 钉死，
   n < 阈值时 low 路径经 sqr_hard_n_ (n > 19 回落 sqr_basecase)。 */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

typedef struct {
    mp_ptr a;
    mp_ptr d;
    mp_size_t n;
} sqr_ctx;

static void sqr_ctx_init(sqr_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)(2 * n + 1) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, n, UINT64_C(0x243f6a8885a308d3));
    c->a[n - 1] |= LIMB_B_2;
}

static void sqr_ctx_free(sqr_ctx* c) {
    lmmp_free(c->a);
    lmmp_free(c->d);
}

static double bench_sqr(void* v) {
    sqr_ctx* c = (sqr_ctx*)v;
    lmmp_sqr_(c->d, c->a, c->n);
    return 0.0;
}

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_SQR_TOOM22_THRESHOLD; }
static void set_threshold(uint64_t v) { lmmp_tune_SQR_TOOM22_THRESHOLD = v; }

static void* make_ctx(uint64_t size, int use_high) {
    sqr_ctx* c = (sqr_ctx*)lmmp_alloc(sizeof(sqr_ctx));
    (void)use_high;
    if (c != NULL)
        sqr_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    sqr_ctx* c = (sqr_ctx*)v;
    if (c != NULL) {
        sqr_ctx_free(c);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    if (use_high)
        set_threshold(size); /* 仅强制根层进入 toom2，递归子问题仍按以下阈值回退 */
    else
        set_threshold(size + 1);
}

int tune_run_sqr_toom22(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "SQR_TOOM22_THRESHOLD";
    spec.low_name = "sqr_hard";
    spec.high_name = "sqr_toom2";
    spec.lo = 20;
    spec.hi = lmmp_tune_SQR_TOOM33_THRESHOLD > 20 ? lmmp_tune_SQR_TOOM33_THRESHOLD - 1 : 60;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_sqr;
    return tune_run_1d(&spec);
}

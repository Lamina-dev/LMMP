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

/* 阈值调优：MUL_TOOM22_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t n;
} mul_ctx;

static void mul_ctx_init(mul_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)(2 * n + 1) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, n, UINT64_C(0x243f6a8885a308d3));
    tune_fill_limbs(c->b, n, UINT64_C(0x13198a2e03707344));
    c->a[n - 1] |= LIMB_B_2;
    c->b[n - 1] |= LIMB_B_2;
}

static void mul_ctx_free(mul_ctx* c) {
    lmmp_free(c->a);
    lmmp_free(c->b);
    lmmp_free(c->d);
}

static double bench_mul_n(void* v) {
    mul_ctx* c = (mul_ctx*)v;
    lmmp_mul_n_(c->d, c->a, c->b, c->n);
    return 0.0;
}

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_MUL_TOOM22_THRESHOLD; }
static void set_threshold(uint64_t v) { lmmp_tune_MUL_TOOM22_THRESHOLD = v; }

static void* make_ctx(uint64_t size, int use_high) {
    mul_ctx* c = (mul_ctx*)lmmp_alloc(sizeof(mul_ctx));
    (void)use_high;
    if (c != NULL)
        mul_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    mul_ctx* c = (mul_ctx*)v;
    if (c != NULL) {
        mul_ctx_free(c);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    if (use_high)
        set_threshold(5);
    else
        set_threshold(size + 1);
}

int tune_run_mul_toom22(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "MUL_TOOM22_THRESHOLD";
    spec.low_name = "mul_basecase";
    spec.high_name = "mul_toom22";
    spec.lo = 5;
    spec.hi = lmmp_tune_MUL_TOOM33_THRESHOLD > 6 ? lmmp_tune_MUL_TOOM33_THRESHOLD - 1 : 60;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_mul_n;
    return tune_run_1d(&spec);
}

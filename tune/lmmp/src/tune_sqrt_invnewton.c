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

/* 阈值调优：SQRT_INVNEWTON_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

#define TUNE_SQRT_FIXED_NA 32

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_SQRT_INVNEWTON_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_SQRT_INVNEWTON_THRESHOLD = v; }

typedef struct {
    mp_ptr a;
    mp_ptr d;
    mp_size_t na;
    mp_size_t nf;
} sqrt_ctx;

/*
 * 实际条件：dstr == NULL 且 nf >= 10*na + T 时使用 invsqrt_newton。
 * 固定 na，样本点 size 表示富余量 slack = nf - 10*na。
 */
static void sqrt_ctx_init(sqrt_ctx* c, uint64_t slack) {
    c->na = TUNE_SQRT_FIXED_NA;
    c->nf = (mp_size_t)(10 * c->na + slack);
    c->a = (mp_ptr)lmmp_alloc((size_t)c->na * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)(c->na + 2 * c->nf + 8) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, c->na, UINT64_C(0x6124c90a4f06a12b));
    c->a[0] |= 1u;
    c->a[c->na - 1] |= LIMB_B_2;
}

static double bench_sqrt(void* v) {
    sqrt_ctx* c = (sqrt_ctx*)v;
    lmmp_sqrt_(c->d, NULL, c->a, c->na, c->nf);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    sqrt_ctx* c = (sqrt_ctx*)lmmp_alloc(sizeof(sqrt_ctx));
    if (c != NULL)
        sqrt_ctx_init(c, size);
    return c;
}

static void free_ctx(void* v) {
    sqrt_ctx* c = (sqrt_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->a);
        lmmp_free(c->d);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    set_threshold(use_high ? 0 : size + 1);
}

int tune_run_sqrt_invnewton(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "SQRT_INVNEWTON_THRESHOLD";
    spec.low_name = "sqrt_divide";
    spec.high_name = "sqrt_invsqrt_newton";
    spec.lo = 0;
    spec.hi = 512;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_sqrt;
    return tune_run_1d(&spec);
}

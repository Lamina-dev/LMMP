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

/* 阈值调优：DIV_DIVIDE_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_DIV_DIVIDE_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_DIV_DIVIDE_THRESHOLD = v; }

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr q;
    mp_size_t n;
} div_ctx;

static void div_ctx_init(div_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)(2 * n + 2) * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->q = (mp_ptr)lmmp_alloc((size_t)(n + 2) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, 2 * n, UINT64_C(0x3c6ef372fe94f82b));
    tune_fill_limbs(c->b, n, UINT64_C(0xa54ff53a5f1d36f1));
    c->a[2 * n - 1] &= ~LIMB_B_2;   /* a < B^(2n)，保证商长度可控 */
    c->b[n - 1] |= LIMB_B_2;        /* 除数规整化 */
    c->a[2 * n] = 0;
    c->a[2 * n + 1] = 0;
}

static double bench_div(void* v) {
    div_ctx* c = (div_ctx*)v;
    (void)lmmp_div_s_(c->q, c->a, 2 * c->n, c->b, c->n);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    div_ctx* c = (div_ctx*)lmmp_alloc(sizeof(div_ctx));
    if (c != NULL)
        div_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    div_ctx* c = (div_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->a);
        lmmp_free(c->b);
        lmmp_free(c->q);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    set_threshold(use_high ? size : size + 1);
}

int tune_run_div_divide(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "DIV_DIVIDE_THRESHOLD";
    spec.low_name = "div_basecase";
    spec.high_name = "div_divide";
    spec.lo = 6;
    spec.hi = 64;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_div;
    return tune_run_1d(&spec);
}

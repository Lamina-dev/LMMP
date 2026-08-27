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

/* 阈值调优：MULLO_BASECASE_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_MULLO_BASECASE_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_MULLO_BASECASE_THRESHOLD = v; }

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t n;
} mullo_ctx;

static void mullo_ctx_init(mullo_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, n, UINT64_C(0xa4093822299f31d0));
    tune_fill_limbs(c->b, n, UINT64_C(0x082efa98ec4e6c89));
    c->a[n - 1] |= LIMB_B_2;
    c->b[n - 1] |= LIMB_B_2;
}

static double bench_mullo(void* v) {
    mullo_ctx* c = (mullo_ctx*)v;
    lmmp_mullo_(c->d, c->a, c->b, c->n);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    mullo_ctx* c = (mullo_ctx*)lmmp_alloc(sizeof(mullo_ctx));
    if (c != NULL)
        mullo_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    mullo_ctx* c = (mullo_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->a);
        lmmp_free(c->b);
        lmmp_free(c->d);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    set_threshold(use_high ? size : size + 1);
}

int tune_run_mullo_basecase(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "MULLO_BASECASE_THRESHOLD";
    spec.low_name = "mullo_basecase";
    spec.high_name = "mullo_dc";
    spec.lo = 4;
    spec.hi = 128;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_mullo;
    return tune_run_1d(&spec);
}

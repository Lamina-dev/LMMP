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

/* 阈值调优：ELEM_MUL_BASECASE_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/impl/ele_mul.h"
static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD; }
static void set_threshold(uint64_t v) { lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD = v; }

typedef struct {
    mp_ptr dst;
    mp_ptr tp;
    ulong* limbs;
    mp_size_t n;
} elem_ctx;

static void elem_ctx_init(elem_ctx* c, mp_size_t n) {
    uint64_t state = UINT64_C(0x6a09e667f3bcc909);
    c->n = n;
    c->dst = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->tp = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->limbs = (ulong*)lmmp_alloc((size_t)n * sizeof(ulong));
    for (mp_size_t i = 0; i < n; ++i) {
        state += UINT64_C(0x9e3779b97f4a7c15);
        c->limbs[i] = (ulong)(state & UINT64_C(0xfffffffe)) | 1u;
    }
}

static double bench_elem(void* v) {
    elem_ctx* c = (elem_ctx*)v;
    (void)lmmp_elem_mul_ulong_(c->dst, c->limbs, c->n, c->tp);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    elem_ctx* c = (elem_ctx*)lmmp_alloc(sizeof(elem_ctx));
    if (c != NULL)
        elem_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    elem_ctx* c = (elem_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->dst);
        lmmp_free(c->tp);
        lmmp_free(c->limbs);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    set_threshold(use_high ? 2 : size + 1);
}

int tune_run_elem_mul(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "ELEM_MUL_BASECASE_THRESHOLD";
    spec.low_name = "elem_basecase";
    spec.high_name = "elem_huffman";
    spec.lo = 2;
    spec.hi = 256;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_elem;
    return tune_run_1d(&spec);
}

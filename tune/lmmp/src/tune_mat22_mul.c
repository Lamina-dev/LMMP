/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published by
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/* 阈值调优：MAT22_MUL_STRASSEN_THRESHOLD（basecase 与 Strassen 的分界） */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/impl/mat22_mul.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = v; }

typedef struct {
    lmmp_mat22_t a;
    lmmp_mat22_t b;
    lmmp_mat22_t d;
    mp_ptr va[4];
    mp_ptr vb[4];
    mp_ptr vd[4];
    mp_ptr tp;
    mp_size_t n;
} mat22_ctx;

static void mat22_ctx_init(mat22_ctx* c, mp_size_t n) {
    uint64_t sa = UINT64_C(0xbb67ae8584caa73b);
    uint64_t sb = UINT64_C(0x3c6ef372fe94f82b);
    c->n = n;
    for (int i = 0; i < 4; ++i) {
        c->va[i] = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
        c->vb[i] = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
        c->vd[i] = (mp_ptr)lmmp_alloc((size_t)(2 * n + 4) * sizeof(mp_limb_t));
        tune_fill_limbs(c->va[i], n, sa + (uint64_t)i * UINT64_C(0x100000001b3));
        tune_fill_limbs(c->vb[i], n, sb + (uint64_t)i * UINT64_C(0x9e3779b97f4a7c15));
        c->va[i][n - 1] |= LIMB_B_2;
        c->vb[i][n - 1] |= LIMB_B_2;
    }
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            c->a.p[i][j] = c->va[2 * i + j];
            c->a.n[i][j] = n;
            c->b.p[i][j] = c->vb[2 * i + j];
            c->b.n[i][j] = n;
            c->d.p[i][j] = c->vd[2 * i + j];
            c->d.n[i][j] = 0;
        }
    }
    c->tp = (mp_ptr)lmmp_alloc((size_t)(8 * (2 * n) + 16) * sizeof(mp_limb_t));
}

static double bench_mat22_mul(void* v) {
    mat22_ctx* c = (mat22_ctx*)v;
    lmmp_mat22_mul_(&c->d, &c->a, &c->b, c->tp);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    mat22_ctx* c = (mat22_ctx*)lmmp_alloc(sizeof(mat22_ctx));
    if (c != NULL)
        mat22_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    mat22_ctx* c = (mat22_ctx*)v;
    if (c != NULL) {
        for (int i = 0; i < 4; ++i) {
            lmmp_free(c->va[i]);
            lmmp_free(c->vb[i]);
            lmmp_free(c->vd[i]);
        }
        lmmp_free(c->tp);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    set_threshold(use_high ? 4 : size + 1);
}

int tune_run_mat22_mul(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "MAT22_MUL_STRASSEN_THRESHOLD";
    spec.low_name = "mat22_mul_basecase";
    spec.high_name = "mat22_mul_strassen";
    spec.lo = 4;
    spec.hi = 32;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_mat22_mul;
    return tune_run_1d(&spec);
}

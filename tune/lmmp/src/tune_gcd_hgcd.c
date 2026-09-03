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
 *  This program is distributed in WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

/* 阈值调优：GCD_HGCD_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_GCD_HGCD_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_GCD_HGCD_THRESHOLD = v; }

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t n;
} gcd_ctx;

/* 随机同长数对（b 顶 limb 减半保证 a>b，欧几里得步数接近最坏情形，
   对两种算法同等公平）。gcd 计算耗时，样本规模控制在中等范围 */
static void gcd_ctx_init(gcd_ctx* c, uint64_t size) {
    c->n = (mp_size_t)size;
    c->a = (mp_ptr)lmmp_alloc((size_t)c->n * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)c->n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)c->n * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, c->n, UINT64_C(0x9e3779b97f4a12b5));
    tune_fill_limbs(c->b, c->n, UINT64_C(0xbf58476d1ce4e5b9));
    c->a[c->n - 1] |= LIMB_B_2;
    c->b[c->n - 1] >>= 1;
    if (c->b[c->n - 1] == 0)
        c->b[c->n - 1] = 1;
}

static double bench_gcd(void* v) {
    gcd_ctx* c = (gcd_ctx*)v;
    (void)lmmp_gcd_(c->d, c->a, c->n, c->b, c->n);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    gcd_ctx* c = (gcd_ctx*)lmmp_alloc(sizeof(gcd_ctx));
    if (c != NULL)
        gcd_ctx_init(c, size);
    return c;
}

static void free_ctx(void* v) {
    gcd_ctx* c = (gcd_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->a);
        lmmp_free(c->b);
        lmmp_free(c->d);
        lmmp_free(c);
    }
}

int tune_run_gcd_hgcd(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "GCD_HGCD_THRESHOLD";
    spec.low_name = "gcd_lehmer";
    spec.high_name = "gcd_hgcd";
    spec.lo = 64;
    spec.hi = 1024;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_gcd;
    return tune_run_1d(&spec);
}

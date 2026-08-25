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

/* 阈值调优：MULHI_MERSENNE_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_MULHI_MERSENNE_THRESHOLD; }
static void set_threshold(uint64_t v) { lmmp_tune_MULHI_MERSENNE_THRESHOLD = v; }

typedef struct {
    mp_ptr a;
    mp_ptr d;
    mp_ptr tp;
    mp_size_t n;
} mulhi_ctx;

static void mulhi_ctx_init(mulhi_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->tp = (mp_ptr)lmmp_alloc((size_t)(5 * (n + 1) / 2 + 2) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, n, UINT64_C(0x84c56a10b2f1132c));
    c->a[0] |= 1u;
    c->a[n - 1] |= LIMB_B_2;
}

/*
 * binvert_n_dc_ 内部通过 binvert_mulhi_ 选择普通乘法高位或梅森变换。
 * 直接以完整函数为黑盒，保持与真实调用路径完全一致。
 */
static double bench_mulhi(void* v) {
    mulhi_ctx* c = (mulhi_ctx*)v;
    lmmp_binvert_n_dc_(c->d, c->a, c->n, c->tp);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    mulhi_ctx* c = (mulhi_ctx*)lmmp_alloc(sizeof(mulhi_ctx));
    if (c != NULL)
        mulhi_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    mulhi_ctx* c = (mulhi_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->a);
        lmmp_free(c->d);
        lmmp_free(c->tp);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    (void)size;
    set_threshold(use_high ? 8 : (UINT64_C(1) << 20));
}

int tune_run_mulhi_mersenne(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "MULHI_MERSENNE_THRESHOLD";
    spec.low_name = "mulhi_mul_n";
    spec.high_name = "mulhi_mersenne";
    spec.lo = 8;
    spec.hi = 1024;
    spec.sample_lo = 16;
    spec.sample_hi = 1024;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_mulhi;
    return tune_run_1d(&spec);
}

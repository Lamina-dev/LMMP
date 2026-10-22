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

/* 阈值调优：MUL_MERSENNE_FFT_THRESHOLD（分发入口内部 dc vs fft 的分界） */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_MUL_MERSENNE_FFT_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_MUL_MERSENNE_FFT_THRESHOLD = v; }

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t rn;
} mulm_ctx;

static void mulm_ctx_init(mulm_ctx* c, mp_size_t rn) {
    c->rn = rn;
    c->a = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, rn, UINT64_C(0x120e3489bce0d51f));
    tune_fill_limbs(c->b, rn, UINT64_C(0x92cab5432210fd9c));
}

static double bench_mers(void* v) {
    mulm_ctx* c = (mulm_ctx*)v;
    lmmp_mul_mersenne_(c->d, c->rn, c->a, c->rn, c->b, c->rn);
    return 0.0;
}

static void free_ctx(void* v) {
    mulm_ctx* c = (mulm_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->a);
        lmmp_free(c->b);
        lmmp_free(c->d);
        lmmp_free(c);
    }
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    mulm_ctx* c = (mulm_ctx*)lmmp_alloc(sizeof(mulm_ctx));
    if (c != NULL) mulm_ctx_init(c, lmmp_fft_next_size_((mp_size_t)size));
    return c;
}

static void apply_path(uint64_t size, int use_high) {
    uint64_t rn = (uint64_t)lmmp_fft_next_size_((mp_size_t)size);
    set_threshold(use_high ? rn : rn + 1);
}

static uint64_t map_size(uint64_t raw) { return (uint64_t)lmmp_fft_next_size_((mp_size_t)raw); }

int tune_run_mul_mersenne_fft(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "MUL_MERSENNE_FFT_THRESHOLD";
    spec.low_name = "mersenne_dc";
    spec.high_name = "mersenne_fft";
    spec.lo = 512;
    spec.hi = 32768;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_mers;
    spec.map_size = map_size;
    return tune_run_1d(&spec);
}

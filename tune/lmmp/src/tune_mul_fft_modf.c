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

/* 阈值调优：MUL_FFT_MODF_THRESHOLD */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_MUL_FFT_MODF_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_MUL_FFT_MODF_THRESHOLD = v; }

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t n;
} fft_ctx;

static void fft_ctx_init(fft_ctx* c, mp_size_t n) {
    c->n = n;
    c->a = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)(2 * n + 1) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, n, UINT64_C(0x9b05688c2b3e6c1f));
    tune_fill_limbs(c->b, n, UINT64_C(0x1f83d9abfb41bd6b));
    c->a[n - 1] |= LIMB_B_2;
    c->b[n - 1] |= LIMB_B_2;
}

static double bench_mul_fft(void* v) {
    fft_ctx* c = (fft_ctx*)v;
    lmmp_mul_fft_(c->d, c->a, c->n, c->b, c->n);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    fft_ctx* c = (fft_ctx*)lmmp_alloc(sizeof(fft_ctx));
    if (c != NULL)
        fft_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    fft_ctx* c = (fft_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->a);
        lmmp_free(c->b);
        lmmp_free(c->d);
        lmmp_free(c);
    }
}

/*
 * 该阈值位于 mul_fermat/sqr_fermat 的递归内部。调优时用完整的
 * lmmp_mul_fft_ 作为黑盒：low 强制内部始终直接乘法，high 强制内部始终
 * 递归 FFT；阈值域 [1,2048] 与实际内部 rn 的工程区间一致。
 */
static void apply_path(uint64_t size, int use_high) {
    (void)size;
    set_threshold(use_high ? 8 : (UINT64_C(1) << 20));
}

int tune_run_mul_fft_modf(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "MUL_FFT_MODF_THRESHOLD";
    spec.low_name = "fermat_direct_mul";
    spec.high_name = "fermat_recursive_fft";
    spec.lo = 128;
    spec.hi = 2048;
    spec.sample_lo = 512;
    spec.sample_hi = 4096;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_mul_fft;
    return tune_run_1d(&spec);
}

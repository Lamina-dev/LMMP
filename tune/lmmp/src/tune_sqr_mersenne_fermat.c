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

/* 阈值调优：SQR_MERSENNE_FERMAT_THRESHOLD（dc 内 +1 半域：直乘折叠 vs fermat 引擎）
 * 负载 size 即半尺寸 n（dc 于 rn=2n 调用，其 +1 半域恰为 n=size） */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_SQR_MERSENNE_FERMAT_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_SQR_MERSENNE_FERMAT_THRESHOLD = v; }

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
    lmmp_sqr_mersenne_dc_(c->d, c->rn, c->a, c->rn);
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
    if (c != NULL) mulm_ctx_init(c, 2 * lmmp_fft_next_size_((mp_size_t)size));
    return c;
}

/* 半尺寸吸附 fft 可行尺寸：未对齐的 n 高路会静默回落 subfold，测不到 fermat */
static uint64_t map_size(uint64_t raw) { return (uint64_t)lmmp_fft_next_size_((mp_size_t)raw); }

static void apply_path(uint64_t size, int use_high) {
    uint64_t n = map_size(size);
    set_threshold(use_high ? n : n + 1);
}

int tune_run_sqr_mersenne_fermat(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "SQR_MERSENNE_FERMAT_THRESHOLD";
    spec.low_name = "dc_subfold";
    spec.high_name = "dc_fermat";
    spec.lo = 384;
    spec.hi = 4096;
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

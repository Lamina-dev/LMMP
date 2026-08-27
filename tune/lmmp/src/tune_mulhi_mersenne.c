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
    mp_ptr dp;
    mp_ptr xp;
    mp_ptr ap;
    mp_ptr tp;
    mp_size_t n;
} mulhi_ctx;

static void mulhi_ctx_init(mulhi_ctx* c, mp_size_t n) {
    c->n = n;
    c->dp = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->xp = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->ap = (mp_ptr)lmmp_alloc((size_t)n * sizeof(mp_limb_t));
    c->tp = (mp_ptr)lmmp_alloc((size_t)(2 * n * sizeof(mp_limb_t)));
    tune_fill_limbs(c->xp, n, UINT64_C(0x84c56a10b2f1132c));
    tune_fill_limbs(c->ap, n, UINT64_C(0x137cbe91211dea3c));
}

/**
 * @brief 计算 [dst,n] = [xp,n]*[ap,n] div B^n
 * @param dst 结果指针
 * @param tp scratch space, need 2*n limbs
 * @warning [xp,n] * [ap,n] mod B^n == 1
 */
static inline void binvert_mulhi_(mp_ptr dst, mp_srcptr xp, mp_srcptr ap, mp_size_t n, mp_ptr tp) {
    if (n < MULHI_MERSENNE_THRESHOLD) {
        lmmp_mul_n_(tp, xp, ap, n);
        lmmp_copy(dst, tp + n, n);
    } else {
        mp_size_t m = lmmp_fft_next_size_((n * 2 + 1) >> 1);
        lmmp_mul_mersenne_(tp, m, xp, n, ap, n);
        lmmp_dec(tp);
        mp_size_t fn = m - n;   // 从 tp+n 开始的长度
        mp_size_t sn = n - fn;  // 从 tp 开始的长度
        lmmp_copy(dst, tp + n, fn);
        lmmp_copy(dst + fn, tp, sn);
    }
}

/*
 * binvert_n_dc_ 内部通过 binvert_mulhi_ 选择普通乘法高位或梅森变换。
 * 直接以完整函数为黑盒，保持与真实调用路径完全一致。
 */
static double bench_mulhi(void* v) {
    mulhi_ctx* c = (mulhi_ctx*)v;
    binvert_mulhi_(c->dp, c->xp, c->ap, c->n, c->tp);
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
        lmmp_free(c->dp);
        lmmp_free(c->xp);
        lmmp_free(c->ap);
        lmmp_free(c->tp);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    (void)size;
    set_threshold(use_high ? size : (UINT64_C(1) << 20));
}

int tune_run_mulhi_mersenne(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "MULHI_MERSENNE_THRESHOLD";
    spec.low_name = "mulhi_mul_n";
    spec.high_name = "mulhi_mersenne";
    spec.lo = 128;
    spec.hi = 1024;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_mulhi;
    return tune_run_1d(&spec);
}

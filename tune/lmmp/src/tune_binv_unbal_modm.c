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

/* 阈值调优：BINVERT_UNBALANCE_MODM_THRESHOLD
 *
 * 语境调优器：binvert_unbalance_ 主循环 k 更新的 mul_n vs 卷绕梅森 cache
 * （模长 next_size((2na+1)/2)≈na + 旋转载出）。k 更新每 na 块执行一次、
 * 块数少（路由限制 4n≥5na，约 ≤5 块），缓存摊薄弱，须以 lmmp_binvert_
 * 不平衡形状黑盒测量。固定 n = 5*na/4（不平衡路径最大比率，块数最多）。
 */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD = v; }

typedef struct {
    mp_ptr a;
    mp_ptr dst;
    mp_size_t na;
    mp_size_t n;
} binv_ctx;

static void binv_ctx_init(binv_ctx* c, mp_size_t na) {
    c->na = na;
    c->n = (5 * na + 3) / 4;
    c->a = (mp_ptr)lmmp_alloc((size_t)na * sizeof(mp_limb_t));
    c->dst = (mp_ptr)lmmp_alloc((size_t)(c->n + 2) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, na, UINT64_C(0x84c56a10b2f1132c));
    c->a[0] |= 1u;
    c->a[na - 1] |= LIMB_B_2;
}

static double bench_binv(void* v) {
    binv_ctx* c = (binv_ctx*)v;
    lmmp_binvert_(c->dst, c->a, c->na, c->n);
    return 0.0;
}

static void free_ctx(void* v) {
    binv_ctx* c = (binv_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->a);
        lmmp_free(c->dst);
        lmmp_free(c);
    }
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    binv_ctx* c = (binv_ctx*)lmmp_alloc(sizeof(binv_ctx));
    if (c != NULL)
        binv_ctx_init(c, (mp_size_t)size);
    return c;
}

/* 真实选择：na >= 阈值 → 卷绕梅森。测量点 size=na：低路 size+1，高路 size。 */
static void apply_path(uint64_t size, int use_high) {
    set_threshold(use_high ? size : size + 1);
}

int tune_run_binv_unbal_modm(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "BINVERT_UNBALANCE_MODM_THRESHOLD";
    spec.low_name = "binv_k_mul_n";
    spec.high_name = "binv_k_mersenne";
    spec.lo = 100;
    spec.hi = 2048;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_binv;
    return tune_run_1d(&spec);
}

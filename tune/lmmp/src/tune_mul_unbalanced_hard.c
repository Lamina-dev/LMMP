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

/* 阈值调优：MUL_UNBALANCED_HARD_THRESHOLD
   不平衡乘法中较短乘数 nb 的分块策略分界：
     low  = PART_SIZE(256) 分块 + mul_basecase 逐列
     high = nb 分块 + 硬编码平衡乘累加 (nb <= LMMP_MUL_HARD_MAX_N)
   样本 size 即 nb (3..19)，na 固定 1024 保证落在多块累积区 (na > PART_SIZE)。
   候选上限 20 恰为"禁用硬编码分块"（nb <= 19 恒不满足门槛）。 */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"

#define UNB_NA 1024

typedef struct {
    mp_ptr a;
    mp_ptr b;
    mp_ptr d;
    mp_size_t na;
    mp_size_t nb;
} unb_ctx;

static void unb_ctx_init(unb_ctx* c, mp_size_t nb) {
    c->na = UNB_NA;
    c->nb = nb;
    c->a = (mp_ptr)lmmp_alloc((size_t)c->na * sizeof(mp_limb_t));
    c->b = (mp_ptr)lmmp_alloc((size_t)nb * sizeof(mp_limb_t));
    c->d = (mp_ptr)lmmp_alloc((size_t)(c->na + nb) * sizeof(mp_limb_t));
    tune_fill_limbs(c->a, c->na, UINT64_C(0x452821e638d01377));
    tune_fill_limbs(c->b, nb, UINT64_C(0xbe5466cf34e90c6c));
    c->a[c->na - 1] |= LIMB_B_2;
    c->b[nb - 1] |= LIMB_B_2;
}

static void unb_ctx_free(unb_ctx* c) {
    lmmp_free(c->a);
    lmmp_free(c->b);
    lmmp_free(c->d);
}

static double bench_unbalanced(void* v) {
    unb_ctx* c = (unb_ctx*)v;
    lmmp_mul_(c->d, c->a, c->na, c->b, c->nb);
    return 0.0;
}

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_MUL_UNBALANCED_HARD_THRESHOLD; }
static void set_threshold(uint64_t v) { lmmp_tune_MUL_UNBALANCED_HARD_THRESHOLD = v; }

static void* make_ctx(uint64_t size, int use_high) {
    unb_ctx* c = (unb_ctx*)lmmp_alloc(sizeof(unb_ctx));
    (void)use_high;
    if (c != NULL)
        unb_ctx_init(c, (mp_size_t)size);
    return c;
}

static void free_ctx(void* v) {
    unb_ctx* c = (unb_ctx*)v;
    if (c != NULL) {
        unb_ctx_free(c);
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    if (use_high)
        set_threshold(size); /* nb >= size -> 硬编码分块 */
    else
        set_threshold(size + 1);
}

int tune_run_mul_unbalanced_hard(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "MUL_UNBALANCED_HARD_THRESHOLD";
    spec.low_name = "mul_basecase_chunk";
    spec.high_name = "mul_hard_chunk";
    spec.lo = 3;
    spec.hi = 20;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_unbalanced;
    return tune_run_1d(&spec);
}

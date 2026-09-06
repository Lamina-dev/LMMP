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

/* 阈值调优：SQRT_INVNEWTON_K_THRESHOLD（斜率） */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

/*
 * 实际条件：dstr == NULL 且 nf >= K*na 时使用 invsqrt_newton。
 * 切换点由斜率 K 单独决定，其最优值随 na 在一定范围波动（乘法算法档位
 * 不同），若仅在单一 na 下调优会把 K 拟合成该 na 的专属值。故每个样本
 * 点聚合一组 na 混合测量，样本点 size 即比例 r（nf = r*na），此时
 * "nf >= K*na" 等价于 "r >= K"，正好沿用一维框架的 TUNE_HIGH_WHEN_GE
 * 语义：阈值 K 即两条路径的比例切换点。
 */
static const mp_size_t TUNE_SQRT_NAS[] = {2, 8, 32, 256};
#define TUNE_SQRT_NA_NUM (sizeof(TUNE_SQRT_NAS) / sizeof(TUNE_SQRT_NAS[0]))

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD = v; }

typedef struct {
    mp_ptr a[TUNE_SQRT_NA_NUM];
    mp_ptr d[TUNE_SQRT_NA_NUM];
    mp_size_t na[TUNE_SQRT_NA_NUM];
    mp_size_t nf[TUNE_SQRT_NA_NUM];
} sqrt_ctx;

/* 样本点 size 表示比例 r = nf/na，对组内每个 na 取 nf = r*na */
static void sqrt_ctx_init(sqrt_ctx* c, uint64_t r) {
    for (size_t i = 0; i < TUNE_SQRT_NA_NUM; ++i) {
        c->na[i] = TUNE_SQRT_NAS[i];
        c->nf[i] = (mp_size_t)r * c->na[i];
        c->a[i] = (mp_ptr)lmmp_alloc((size_t)c->na[i] * sizeof(mp_limb_t));
        c->d[i] = (mp_ptr)lmmp_alloc((size_t)(c->na[i] + 2 * c->nf[i] + 8) * sizeof(mp_limb_t));
        tune_fill_limbs(c->a[i], c->na[i], UINT64_C(0x6124c90a4f06a12b) ^ (uint64_t)i);
        c->a[i][0] |= 1u;
        c->a[i][c->na[i] - 1] |= LIMB_B_2;
    }
}

static double bench_sqrt(void* v) {
    sqrt_ctx* c = (sqrt_ctx*)v;
    for (size_t i = 0; i < TUNE_SQRT_NA_NUM; ++i)
        lmmp_sqrt_(c->d[i], NULL, c->a[i], c->na[i], c->nf[i]);
    return 0.0;
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    sqrt_ctx* c = (sqrt_ctx*)lmmp_alloc(sizeof(sqrt_ctx));
    if (c != NULL)
        sqrt_ctx_init(c, size);
    return c;
}

static void free_ctx(void* v) {
    sqrt_ctx* c = (sqrt_ctx*)v;
    if (c != NULL) {
        for (size_t i = 0; i < TUNE_SQRT_NA_NUM; ++i) {
            lmmp_free(c->a[i]);
            lmmp_free(c->d[i]);
        }
        lmmp_free(c);
    }
}

static void apply_path(uint64_t size, int use_high) {
    /* r >= K 时走 newton：high 取 K=r（含），low 取 K=r+1（不含） */
    set_threshold(use_high ? size : size + 1);
}

int tune_run_sqrt_invnewton(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "SQRT_INVNEWTON_K_THRESHOLD";
    spec.low_name = "sqrt_divide";
    spec.high_name = "sqrt_invsqrt_newton";
    spec.lo = 6;
    spec.hi = 48;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_sqrt;
    return tune_run_1d(&spec);
}

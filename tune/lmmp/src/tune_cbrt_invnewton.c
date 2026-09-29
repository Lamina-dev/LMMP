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

/* 阈值调优：CBRT_INVNEWTON_NF_MIN + CBRT_INVNEWTON_K_THRESHOLD（AND 复合切换） */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

/*
 * 实际条件：dstr == NULL 且 nf >= K*na 且 nf >= NF_MIN 时使用 invcbrt_newton。
 * 等耗时边界为水平段（na <= 12，交叉点 ~2000，newton 固定开销主导）拼接
 * 射线段（na ∈ [48,384]，交叉点 nf* ≈ 斜率*na，两算法增量成本比），两阈值
 * 各管一段，须分别调优且互不干扰：
 *   NF_MIN 段：小 na 样本（K*na << 交叉点），apply 强制 K=1 使射线项恒真，
 *              样本点 size 即 nf，"nf >= NF_MIN" 沿用 TUNE_HIGH_WHEN_GE。
 *   K 段：射线区 na 混合样本，apply 强制 NF_MIN=0 使下限项恒真，样本点
 *         size 即比例 r = nf/na（nf = r*na），"r >= K" 同样是 GE 语义。
 * 每个样本点聚合一组 na 混合测量，避免把阈值拟合成单一 na 的专属值。
 */
#define TUNE_CBRT_CTX_NUM 4

typedef struct {
    mp_ptr a[TUNE_CBRT_CTX_NUM];
    mp_ptr d[TUNE_CBRT_CTX_NUM];
    mp_size_t na[TUNE_CBRT_CTX_NUM];
    mp_size_t nf[TUNE_CBRT_CTX_NUM];
} cbrt_ctx;

/* nf_is_ratio != 0 时 nf = size*na[i]（K 段），否则 nf = size（NF_MIN 段） */
static void cbrt_ctx_fill(cbrt_ctx* c, const mp_size_t* nas, size_t nnum, uint64_t size, int nf_is_ratio) {
    for (size_t i = 0; i < nnum; ++i) {
        c->na[i] = nas[i];
        c->nf[i] = nf_is_ratio ? (mp_size_t)size * nas[i] : (mp_size_t)size;
        c->a[i] = (mp_ptr)lmmp_alloc((size_t)c->na[i] * sizeof(mp_limb_t));
        c->d[i] = (mp_ptr)lmmp_alloc((size_t)(c->na[i] / 3 + c->nf[i] + 16) * sizeof(mp_limb_t));
        tune_fill_limbs(c->a[i], c->na[i], UINT64_C(0x71a4bb6f1e2c9d03) ^ (uint64_t)i);
        c->a[i][0] |= 1u;
        c->a[i][c->na[i] - 1] |= LIMB_B_2;
    }
}

static void free_ctx(void* v) {
    cbrt_ctx* c = (cbrt_ctx*)v;
    if (c != NULL) {
        for (size_t i = 0; i < TUNE_CBRT_CTX_NUM; ++i) {
            lmmp_free(c->a[i]);
            lmmp_free(c->d[i]);
        }
        lmmp_free(c);
    }
}

static double bench_cbrt(void* v) {
    cbrt_ctx* c = (cbrt_ctx*)v;
    for (size_t i = 0; i < TUNE_CBRT_CTX_NUM; ++i)
        if (c->a[i] != NULL) lmmp_cbrt_(c->d[i], NULL, c->a[i], c->na[i], c->nf[i]);
    return 0.0;
}

/* ---------- 第一段：NF_MIN（小 na 水平交叉点） ---------- */

static const mp_size_t TUNE_CBRT_NF_NAS[] = {3, 6, 12};

static uint64_t get_nfmin(void) { return lmmp_tune_CBRT_INVNEWTON_NF_MIN; }

static void set_nfmin(uint64_t v) { lmmp_tune_CBRT_INVNEWTON_NF_MIN = v; }

static void* make_ctx_nf(uint64_t size, int use_high) {
    (void)use_high;
    cbrt_ctx* c = (cbrt_ctx*)lmmp_alloc(sizeof(cbrt_ctx));
    if (c != NULL) {
        memset(c, 0, sizeof(*c));
        cbrt_ctx_fill(c, TUNE_CBRT_NF_NAS, 3, size, 0);
    }
    return c;
}

static void apply_path_nf(uint64_t size, int use_high) {
    /* K=1：射线项 nf>=na 对全部样本恒真，仅由 NF_MIN 决定路径 */
    lmmp_tune_CBRT_INVNEWTON_K_THRESHOLD = 1;
    lmmp_tune_CBRT_INVNEWTON_NF_MIN = use_high ? size : size + 1;
}

static int tune_cbrt_nfmin_phase(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "CBRT_INVNEWTON_NF_MIN";
    spec.low_name = "cbrt_divide";
    spec.high_name = "cbrt_invsqrt_newton";
    spec.lo = 256;
    spec.hi = 8192;
    spec.sample_lo = 512;
    spec.sample_hi = 4096;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_nfmin;
    spec.set = set_nfmin;
    spec.apply_path = apply_path_nf;
    spec.make_ctx = make_ctx_nf;
    spec.free_ctx = free_ctx;
    spec.bench = bench_cbrt;
    return tune_run_1d(&spec);
}

/* ---------- 第二段：K（射线区斜率） ---------- */

static const mp_size_t TUNE_CBRT_K_NAS[] = {48, 96, 192, 384};

static uint64_t get_k(void) { return lmmp_tune_CBRT_INVNEWTON_K_THRESHOLD; }

static void set_k(uint64_t v) { lmmp_tune_CBRT_INVNEWTON_K_THRESHOLD = v; }

static void* make_ctx_k(uint64_t size, int use_high) {
    (void)use_high;
    cbrt_ctx* c = (cbrt_ctx*)lmmp_alloc(sizeof(cbrt_ctx));
    if (c != NULL) {
        memset(c, 0, sizeof(*c));
        cbrt_ctx_fill(c, TUNE_CBRT_K_NAS, 4, size, 1);
    }
    return c;
}

static void apply_path_k(uint64_t size, int use_high) {
    /* NF_MIN=0：下限项恒真，仅由 K 决定路径（最小样本 nf=24*48 亦满足
     * newton 契约 3nf >= 2na+3） */
    lmmp_tune_CBRT_INVNEWTON_NF_MIN = 0;
    lmmp_tune_CBRT_INVNEWTON_K_THRESHOLD = use_high ? size : size + 1;
}

static int tune_cbrt_k_phase(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "CBRT_INVNEWTON_K_THRESHOLD";
    spec.low_name = "cbrt_divide";
    spec.high_name = "cbrt_invsqrt_newton";
    spec.lo = 16;
    spec.hi = 128;
    spec.sample_lo = 24;
    spec.sample_hi = 96;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_k;
    spec.set = set_k;
    spec.apply_path = apply_path_k;
    spec.make_ctx = make_ctx_k;
    spec.free_ctx = free_ctx;
    spec.bench = bench_cbrt;
    return tune_run_1d(&spec);
}

int tune_run_cbrt_invnewton(void) {
    const uint64_t k_saved = get_k();
    int rc = tune_cbrt_nfmin_phase();
    set_k(k_saved); /* 第一段 apply 将 K 置 1，恢复后再进第二段 */
    if (rc != 0)
        return rc;
    const uint64_t nfmin_tuned = get_nfmin();
    rc = tune_cbrt_k_phase();
    /* 第二段 apply 将 NF_MIN 置 0，恢复第一段结果保持运行时状态一致 */
    set_nfmin(nfmin_tuned);
    return rc;
}

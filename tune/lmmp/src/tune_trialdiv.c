/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
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

/* 阈值调优：TRIALDIV_PREINV_THRESHOLD / TRIALDIV_K8_THRESHOLD
 *
 * 直接测量三档取模核（mod_preinv_1_ / mod_4chunk_1_ / mod_8chunk_1_），
 * 不经 lmmp_trialdiv_ 整腿——块循环、r%p 检查、惰性表初始化与分派开销
 * 对两条路径完全对称，在等耗时方程中相消，故交叉点不变而测量噪声更小。
 *
 * 除数取代表性块积（2^47 档首个块积，k=clz(t)=16 走 mod_preinv_1_ 的
 * 未归一化主路径）：Hensel 折叠与蒙域合并全是全宽 64 位运算，成本与 d 的
 * 位宽无关；preinv 循环成本仅按 k=0/k>0 分两档，块表内 k∈[1,17] 均在
 * 未归一化档，单点 d 即可代表。预处理（inv/k 与 dinv/r2/base/mi）每 ctx
 * 现算一次并缓存，与 trialdiv 全局表的做法一致（预处理外提口径）。
 *
 * 两个阈值各做一个一维任务：PREINV 任务 low=preinv 核 high=4chunk 核；
 * K8 任务 low=4chunk 核 high=8chunk 核。bench 不经过分派，阈值变量仅由
 * 框架自动设置（对核调用无影响）。两任务独立寻优；若极端噪声下解出
 * PREINV > K8，分派退化为 preinv|K8 双档（K4 带空），语义仍正确，仅中段
 * 让渡，无正确性风险。
 */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/impl/trialdiv.h"

/* 代表性块积：3·5·7·… 打包越过 2^47-1 的首块（约 2^47.08，k=16） */
#define TUNE_TRIALDIV_D UINT64_C(0x08a5b6470af95)

enum { LEG_PREINV = 0, LEG_K4 = 1, LEG_K8 = 2 };

typedef struct {
    mp_ptr np;
    mp_size_t nn;
    int leg;
    /* preinv 组参数 */
    mp_limb_t inv, k;
    /* chunk 组参数（同 trialdiv_init_chunk_） */
    mp_limb_t dinv, r2, base, mi;
} trialdiv_ctx;

/** @brief 求 B^nb mod t（移位对消 + div_1_s_，同 trialdiv.c 的 trialdiv_bpow_） */
static mp_limb_t trialdiv_bpow_(mp_limb_t t, uint nb) {
    mp_limb_t m = t;
    mp_bitcnt_t k = 0;
    while ((m & LIMB_B_2) == 0) {
        m <<= 1;
        ++k;
    }
    mp_limb_t r[3] = {0, 0, 0}, q[2];
    r[nb] = (mp_limb_t)1 << k;
    lmmp_div_1_s_(q, r, (mp_size_t)(nb + 1), m);
    return r[0] >> k;
}

/** @brief 构造指定档位的计时上下文：归一化随机被除数 + 块积 d 的全套预处理 */
static void* trialdiv_make_ctx_leg(uint64_t size, int leg) {
    trialdiv_ctx* c = (trialdiv_ctx*)lmmp_alloc(sizeof(trialdiv_ctx));
    if (c == NULL) return NULL;
    c->nn = (mp_size_t)size;
    c->leg = leg;
    c->np = (mp_ptr)lmmp_alloc((size_t)c->nn * sizeof(mp_limb_t));
    if (c->np == NULL) {
        lmmp_free(c);
        return NULL;
    }
    tune_fill_limbs(c->np, c->nn, UINT64_C(0x6a09e667f3bcc909) ^ (uint64_t)size);
    c->np[c->nn - 1] |= LIMB_B_2;
    c->k = (mp_limb_t)lmmp_leading_zeros_(TUNE_TRIALDIV_D);
    c->inv = lmmp_inv_1_(TUNE_TRIALDIV_D << c->k);
    c->dinv = lmmp_binvert_ulong_(TUNE_TRIALDIV_D);
    c->mi = -c->dinv;
    c->r2 = trialdiv_bpow_(TUNE_TRIALDIV_D, 2);
    c->base = trialdiv_bpow_(TUNE_TRIALDIV_D, 1);
    return c;
}

/** @brief 释放计时上下文 */
static void trialdiv_free_ctx(void* v) {
    trialdiv_ctx* c = (trialdiv_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->np);
        lmmp_free(c);
    }
}

/**
 * @brief 计时体：按 ctx 选定的一条取模核跑一次
 * @note 三个核都是无副作用的纯函数，返回值一丢弃，LTO 之下整个调用可被
 *       合法消除——空 asm 屏障逼编译器把返回值交出来，防整段计时被优化空
 */
static double trialdiv_bench(void* v) {
    trialdiv_ctx* c = (trialdiv_ctx*)v;
    mp_limb_t r;
    switch (c->leg) {
    case LEG_PREINV:
        r = lmmp_mod_preinv_1_(c->np, c->nn, TUNE_TRIALDIV_D, c->inv, c->k);
        break;
    case LEG_K4:
        r = lmmp_mod_4chunk_1_(c->np, c->nn, TUNE_TRIALDIV_D, c->dinv, c->r2, c->base, c->mi);
        break;
    default:
        r = lmmp_mod_8chunk_1_(c->np, c->nn, TUNE_TRIALDIV_D, c->dinv, c->r2, c->base, c->mi);
        break;
    }
    __asm__ __volatile__("" : : "r"(r));
    return 0.0;
}

/* ---------------- TRIALDIV_PREINV_THRESHOLD：preinv 核 vs K4 核 ---------------- */

/** @brief 读 TRIALDIV_PREINV_THRESHOLD 阈值变量 */
static uint64_t trialdiv_preinv_get(void) {
    return (uint64_t)lmmp_tune_TRIALDIV_PREINV_THRESHOLD;
}
/** @brief 写 TRIALDIV_PREINV_THRESHOLD 阈值变量 */
static void trialdiv_preinv_set(uint64_t v) { lmmp_tune_TRIALDIV_PREINV_THRESHOLD = v; }

/** @brief PREINV 任务的上下文工厂：low 走 preinv 核，high 走 K4 核 */
static void* trialdiv_make_ctx_preinv(uint64_t size, int use_high) {
    return trialdiv_make_ctx_leg(size, use_high ? LEG_K4 : LEG_PREINV);
}

/** @brief 一维寻优 TRIALDIV_PREINV_THRESHOLD（preinv 核 vs K4 核） */
int tune_run_trialdiv_preinv(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "TRIALDIV_PREINV_THRESHOLD";
    spec.low_name = "mod_preinv_1";
    spec.high_name = "mod_4chunk";
    spec.lo = 2;
    spec.hi = 192;
    spec.sample_lo = 2;
    spec.sample_hi = 256;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = trialdiv_preinv_get;
    spec.set = trialdiv_preinv_set;
    spec.make_ctx = trialdiv_make_ctx_preinv;
    spec.free_ctx = trialdiv_free_ctx;
    spec.bench = trialdiv_bench;
    return tune_run_1d(&spec);
}

/* ---------------- TRIALDIV_K8_THRESHOLD：K4 核 vs K8 核 ---------------- */

/** @brief 读 TRIALDIV_K8_THRESHOLD 阈值变量 */
static uint64_t trialdiv_k8_get(void) { return (uint64_t)lmmp_tune_TRIALDIV_K8_THRESHOLD; }
/** @brief 写 TRIALDIV_K8_THRESHOLD 阈值变量 */
static void trialdiv_k8_set(uint64_t v) { lmmp_tune_TRIALDIV_K8_THRESHOLD = v; }

/** @brief K8 任务的上下文工厂：low 走 K4 核，high 走 K8 核 */
static void* trialdiv_make_ctx_k8(uint64_t size, int use_high) {
    return trialdiv_make_ctx_leg(size, use_high ? LEG_K8 : LEG_K4);
}

/** @brief 一维寻优 TRIALDIV_K8_THRESHOLD（K4 核 vs K8 核） */
int tune_run_trialdiv_k8(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "TRIALDIV_K8_THRESHOLD";
    spec.low_name = "mod_4chunk";
    spec.high_name = "mod_8chunk";
    spec.lo = 16;
    spec.hi = 512;
    spec.sample_lo = 16;
    spec.sample_hi = 640;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = trialdiv_k8_get;
    spec.set = trialdiv_k8_set;
    spec.make_ctx = trialdiv_make_ctx_k8;
    spec.free_ctx = trialdiv_free_ctx;
    spec.bench = trialdiv_bench;
    return tune_run_1d(&spec);
}

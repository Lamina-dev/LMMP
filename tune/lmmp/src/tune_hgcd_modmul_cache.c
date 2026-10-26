/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

/* 阈值调优：HGCD_MODMUL_CACHE_THRESHOLD（hgcd 矩阵模乘：分发不缓存 vs 梅森缓存）
 *
 * 黑盒 lmmp_gcd_ 对该阈值区分度不足（2026-10-21 本机探针：nocache/cache
 * = 0.90-1.01，模乘段在整体 gcd 中占比过低），故内嵌 apply_mod 的四次
 * 模乘块直测；矩阵元素长度取 hn+2 量级（rn/2 附近），与 hgcd 递归矩阵
 * 的实际形状一致。与 src/lmmp/numth/hgcd.c 的模乘段同步维护。
 */

#include "lmmp_tune_internal.h"
#include "lmmp_tune.h"

#include "lmmp/impl/mparam.h"
#include "lmmp/impl/mul_cache.h"
#include "lmmp/lmmpn.h"

static uint64_t get_threshold(void) { return (uint64_t)lmmp_tune_HGCD_MODMUL_CACHE_THRESHOLD; }

static void set_threshold(uint64_t v) { lmmp_tune_HGCD_MODMUL_CACHE_THRESHOLD = v; }

typedef struct {
    mp_ptr fa;
    mp_ptr fb;
    mp_ptr m00;
    mp_ptr m01;
    mp_ptr m10;
    mp_ptr m11;
    mp_ptr t00;
    mp_ptr t01;
    mp_ptr t10;
    mp_ptr t11;
    mp_size_t rn;
    mp_size_t l00, l01, l10, l11; /* 矩阵元素真实长度（略短于 ml，补零对齐） */
    mp_size_t ml1, ml2;
} hgcd_mm_ctx;

static void hgcd_mm_ctx_init(hgcd_mm_ctx* c, mp_size_t rn) {
    c->rn = rn;
    c->ml1 = rn / 2;
    c->ml2 = rn / 2 + 2;
    c->l00 = c->ml1 - 1;
    c->l01 = c->ml1;
    c->l10 = c->ml2 - 2;
    c->l11 = c->ml2;
    c->fa = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    c->fb = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    c->m00 = (mp_ptr)lmmp_alloc((size_t)c->ml1 * sizeof(mp_limb_t));
    c->m01 = (mp_ptr)lmmp_alloc((size_t)c->ml1 * sizeof(mp_limb_t));
    c->m10 = (mp_ptr)lmmp_alloc((size_t)c->ml2 * sizeof(mp_limb_t));
    c->m11 = (mp_ptr)lmmp_alloc((size_t)c->ml2 * sizeof(mp_limb_t));
    c->t00 = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    c->t01 = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    c->t10 = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    c->t11 = (mp_ptr)lmmp_alloc((size_t)rn * sizeof(mp_limb_t));
    tune_fill_limbs(c->fa, rn, UINT64_C(0x51d7a3f29e8b4c60));
    tune_fill_limbs(c->fb, rn, UINT64_C(0x2c94e6b1d05f3877));
    tune_fill_limbs(c->m00, c->l00, UINT64_C(0x7f30c5a92d1e84bb));
    tune_fill_limbs(c->m01, c->l01, UINT64_C(0x48e0b7c3f6195a2d));
    tune_fill_limbs(c->m10, c->l10, UINT64_C(0x93f5d2c8e04b7613));
    tune_fill_limbs(c->m11, c->l11, UINT64_C(0x6a1b8e5f0d3c9247));
}

static void free_ctx(void* v) {
    hgcd_mm_ctx* c = (hgcd_mm_ctx*)v;
    if (c != NULL) {
        lmmp_free(c->fa);
        lmmp_free(c->fb);
        lmmp_free(c->m00);
        lmmp_free(c->m01);
        lmmp_free(c->m10);
        lmmp_free(c->m11);
        lmmp_free(c->t00);
        lmmp_free(c->t01);
        lmmp_free(c->t10);
        lmmp_free(c->t11);
        lmmp_free(c);
    }
}

static void* make_ctx(uint64_t size, int use_high) {
    (void)use_high;
    hgcd_mm_ctx* c = (hgcd_mm_ctx*)lmmp_alloc(sizeof(hgcd_mm_ctx));
    /* 缓存档仅 fft 可行尺寸可用，负载吸附到 next_size（与 hgcd.c 取 rn 一致） */
    if (c != NULL) hgcd_mm_ctx_init(c, lmmp_fft_next_size_((mp_size_t)size));
    return c;
}

static double bench_modmul(void* v) {
    hgcd_mm_ctx* c = (hgcd_mm_ctx*)v;
    /* 补零对齐（矩阵元素真实长度可短可零，缓存版须等长） */
    lmmp_zero(c->m00 + c->l00, c->ml1 - c->l00);
    lmmp_zero(c->m01 + c->l01, c->ml1 - c->l01);
    lmmp_zero(c->m10 + c->l10, c->ml2 - c->l10);
    lmmp_zero(c->m11 + c->l11, c->ml2 - c->l11);
    if (c->rn >= HGCD_MODMUL_CACHE_THRESHOLD) {
        fft_gr_cache ca, cb;
        lmmp_mul_mersenne_cache_init_(c->t11, c->rn, c->m11, c->ml2, c->fa, c->rn, &ca);
        lmmp_mul_mersenne_cache_(c->t10, c->m10, &ca);
        lmmp_mul_mersenne_cache_init_(c->t01, c->rn, c->m01, c->ml1, c->fb, c->rn, &cb);
        lmmp_mul_mersenne_cache_(c->t00, c->m00, &cb);
        lmmp_fft_gr_cache_free_(&ca);
        lmmp_fft_gr_cache_free_(&cb);
    } else {
        lmmp_mul_mersenne_(c->t11, c->rn, c->fa, c->rn, c->m11, c->ml2);
        lmmp_mul_mersenne_(c->t10, c->rn, c->fa, c->rn, c->m10, c->ml2);
        lmmp_mul_mersenne_(c->t01, c->rn, c->fb, c->rn, c->m01, c->ml1);
        lmmp_mul_mersenne_(c->t00, c->rn, c->fb, c->rn, c->m00, c->ml1);
    }
    return 0.0;
}

static void apply_path(uint64_t size, int use_high) {
    uint64_t rn = (uint64_t)lmmp_fft_next_size_((mp_size_t)size);
    set_threshold(use_high ? rn : rn + 1);
}

static uint64_t map_size(uint64_t raw) { return (uint64_t)lmmp_fft_next_size_((mp_size_t)raw); }

int tune_run_hgcd_modmul_cache(void) {
    tune_1d_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    spec.macro_name = "HGCD_MODMUL_CACHE_THRESHOLD";
    spec.low_name = "modmul_mers";
    spec.high_name = "modmul_cache";
    spec.lo = 512;
    spec.hi = 16384;
    spec.pred = TUNE_HIGH_WHEN_GE;
    spec.get = get_threshold;
    spec.set = set_threshold;
    spec.apply_path = apply_path;
    spec.make_ctx = make_ctx;
    spec.free_ctx = free_ctx;
    spec.bench = bench_modmul;
    spec.map_size = map_size;
    return tune_run_1d(&spec);
}

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
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

/* Toom 乘法入口契约边界测试（11 核 + 3 不平衡外层，toom93 核已删除）：
   1) 各核 nb 尺寸门起逐 nb 扫描保守线性比例带（入口断言的整数化，带内
      形状全部落在结构合法域内；42 上沿为余类精确式），直调形状仍经切分
      谓词复核；
   2) 每 nb 取带两端闭边界与中间点；
   3) 尺寸门-1 与带外形状只走 lmmp_mul_ 完整分派验证路由正确，不直调违约形状；
   4) 大尺寸比例带内部直调 + 洞区经分派。 */

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

typedef void (*toom_fn)(mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t);

/* 直调内核 vs basecase 精确对拍 + 模检查 */
static void check_kernel(toom_fn fn, mp_size_t na, mp_size_t nb, u64* seed, const char* tag) {
    mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
    mp_ptr dst = alloc_limbs(na + nb), ref = alloc_limbs(na + nb);
    random_limbs(a, na, seed);
    random_limbs(b, nb, seed);
    fn(dst, a, na, b, nb);
    lmmp_mul_basecase_(ref, a, na, b, nb);
    bool ok = memcmp(dst, ref, (size_t)(na + nb) * sizeof(mp_limb_t)) == 0;
    if (!ok || !mod_is_product(dst, na + nb, a, na, b, nb))
        TEST_CHECK_MSG(false, tag);
    else
        TEST_CHECK(true);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(dst);
    lmmp_free(ref);
}

/* 完整分派 vs basecase（契约外沿/小洞形状只走分派，不直调） */
static void check_dispatch(mp_size_t na, mp_size_t nb, u64* seed, const char* tag) {
    mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
    mp_ptr dst = alloc_limbs(na + nb), ref = alloc_limbs(na + nb);
    random_limbs(a, na, seed);
    random_limbs(b, nb, seed);
    lmmp_mul_(dst, a, na, b, nb);
    lmmp_mul_basecase_(ref, a, na, b, nb);
    bool ok = memcmp(dst, ref, (size_t)(na + nb) * sizeof(mp_limb_t)) == 0;
    if (!ok || !mod_is_product(dst, na + nb, a, na, b, nb))
        TEST_CHECK_MSG(false, tag);
    else
        TEST_CHECK(true);
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(dst);
    lmmp_free(ref);
}

/* ---- 切分谓词：与内核切分公式逐字一致，判定结构合法（reach） ---- */

static bool reach22(mp_size_t na, mp_size_t nb) {
    mp_size_t s = na >> 1, n = na - s, t = nb - n;
    return s > 0 && t > 0 && t <= n && s + t > n;
}

static bool reach32(mp_size_t na, mp_size_t nb) {
    mp_size_t n = 1 + (2 * na >= 3 * nb ? (na - 1) / 3 : (nb - 1) >> 1);
    mp_size_t s = na - 2 * n, t = nb - n;
    return s > 0 && s <= n && t > 0 && t <= n && s + t >= n + 2;
}

static bool reach33(mp_size_t na, mp_size_t nb) {
    mp_size_t n = (na + 2) / 3, s = na - 2 * n, t = nb - 2 * n;
    return s > 0 && s <= n && t > 0 && t <= n;
}

static bool reach42(mp_size_t na, mp_size_t nb) {
    mp_size_t n = na >= 2 * nb ? (na + 3) >> 2 : (nb + 1) >> 1;
    mp_size_t s = na - 3 * n, t = nb - n;
    return s > 0 && s <= n && t > 0 && t <= n && s + t >= n + 2;
}

static bool reach43(mp_size_t na, mp_size_t nb) {
    mp_size_t n = 1 + (3 * na >= 4 * nb ? (na - 1) >> 2 : (nb - 1) / 3);
    mp_size_t s = na - 3 * n, t = nb - 2 * n;
    return s > 0 && s <= n && t > 0 && t <= n && s + t >= 5;
}

static bool reach44(mp_size_t na, mp_size_t nb) {
    mp_size_t n = (na + 3) >> 2, s = na - 3 * n, t = nb - 3 * n;
    return s > 0 && s <= n && t > 0 && t <= n && s >= t;
}

static bool reach52(mp_size_t na, mp_size_t nb) {
    mp_size_t n = 1 + (2 * na >= 5 * nb ? (na - 1) / 5 : (nb - 1) >> 1);
    mp_size_t s = na - 4 * n, t = nb - n;
    return s > 0 && s <= n && t > 0 && t <= n && s + t >= 5;
}

static bool reach53(mp_size_t na, mp_size_t nb) {
    mp_size_t n = 1 + (3 * na >= 5 * nb ? (na - 1) / 5 : (nb - 1) / 3);
    mp_size_t s = na - 4 * n, t = nb - 2 * n;
    return s > 0 && s <= n && t > 0 && t <= n;
}

static bool reach62(mp_size_t na, mp_size_t nb) {
    mp_size_t n = 1 + (na >= 3 * nb ? (na - 1) / 6 : (nb - 1) >> 1);
    mp_size_t s = na - 5 * n, t = nb - n;
    return s > 0 && s <= n && t > 0 && t <= n;
}

static bool reach6h(mp_size_t na, mp_size_t nb) {
    mp_size_t n, s, t;
    int half, p, q;
    if (na * 17 < 18 * nb) {
        n = 1 + (na - 1) / 6;
        p = q = 5;
        s = na - 5 * n;
        t = nb - 5 * n;
        half = 0;
    } else {
        if (na * 90 < 119 * nb) { p = 7; q = 6; }
        else if (na * 85 < 126 * nb) { p = 7; q = 5; }
        else if (na * 18 < 34 * nb) { p = 8; q = 5; }
        else if (na * 17 < 36 * nb) { p = 8; q = 4; }
        else { p = 9; q = 4; }
        half = (p ^ q) & 1;
        n = 1 + ((mp_size_t)q * na >= (mp_size_t)p * nb ? (na - 1) / (mp_size_t)p
                                                        : (nb - 1) / (mp_size_t)q);
        p--; q--;
        s = na - (mp_size_t)p * n;
        t = nb - (mp_size_t)q * n;
        if (half) {
            if (na <= (mp_size_t)p * n) { p--; s += n; half = 0; }
            else if (nb <= (mp_size_t)q * n) { q--; t += n; half = 0; }
        }
    }
    /* eval_pm1 k>=4（q==3 走 dgr3 合法）；couple off<n */
    return s > 0 && s <= n && t > 0 && t <= n && s + t + 3 * (mp_size_t)half > 3 && n > 2 &&
           2 * (1 + (mp_size_t)half) < n && p >= 4 && q >= 3 && na < 3 * nb;
}

static bool reach8h(mp_size_t na, mp_size_t nb) {
    mp_size_t n, s, t;
    int half, p, q;
    if (na == nb || na * 10 < 21 * (nb / 2)) {
        n = 1 + (na - 1) / 8;
        p = q = 7;
        s = na - 7 * n;
        t = nb - 7 * n;
        half = 0;
    } else {
        if (na * 13 < 16 * nb) { p = 9; q = 8; }
        else if (na * 10 < 27 * (nb / 2)) { p = 9; q = 7; }
        else if (na * 10 < 33 * (nb / 2)) { p = 10; q = 7; }
        else if (na * 4 < 7 * nb) { p = 10; q = 6; }
        else if (na * 6 < 13 * nb) { p = 11; q = 6; }
        else if (na * 4 < 9 * nb) { p = 11; q = 5; }
        else if (na * 7 < 20 * nb) { p = 12; q = 5; }
        else if (na * 9 < 28 * nb) { p = 12; q = 4; }
        else { p = 13; q = 4; }
        half = (p + q) & 1;
        n = 1 + ((mp_size_t)q * na >= (mp_size_t)p * nb ? (na - 1) / (mp_size_t)p
                                                        : (nb - 1) / (mp_size_t)q);
        p--; q--;
        s = na - (mp_size_t)p * n;
        t = nb - (mp_size_t)q * n;
        if (half) {
            if (na <= (mp_size_t)p * n) { p--; s += n; half = 0; }
            else if (nb <= (mp_size_t)q * n) { q--; t += n; half = 0; }
        }
    }
    /* eval_pm1 k>=4（q==3 走 dgr3 合法）；couple off<n */
    return s > 0 && s <= n && t > 0 && t <= n && s + t + 3 * (mp_size_t)half > 3 && n > 2 &&
           3 * (1 + (mp_size_t)half) < n && p >= 4 && q >= 3;
}

/* ---- 逐核边界扫描：nb=门..门+17，比例带内逐 na 过滤后取两端+中间 ---- */

typedef bool (*reach_fn)(mp_size_t, mp_size_t);

/* band_lo/band_hi：入口断言比例带的整数边界（含端点，洞由 reach 过滤）。
   偏移在除法前加入分子：lo = (lo_mul*nb + lo_off)/lo_div，保证奇数 nb 时上界
   精确落在 floor((k*nb+c)/div) 类带端点（33 的 (3nb-3)/2 即此形）。 */
static void sweep(const char* tag, toom_fn fn, reach_fn reach, mp_size_t gate,
                  mp_size_t band_lo_mul, mp_size_t band_lo_div, mp_size_t band_lo_off,
                  mp_size_t band_hi_mul, mp_size_t band_hi_div, mp_size_t band_hi_off,
                  u64 seed) {
    int checked = 0;
    for (mp_size_t nb = gate; nb <= gate + 17; ++nb) {
        long long hi_num = (long long)band_hi_mul * nb + band_hi_off;
        if (hi_num < 0)
            continue;  /* 带为空（mp_size_t 无符号，负上界会回绕成死循环） */
        mp_size_t lo = (band_lo_mul * nb + band_lo_off) / band_lo_div;
        mp_size_t hi = (mp_size_t)(hi_num / band_hi_div);
        if (lo < nb)
            lo = nb;
        /* 过滤后合法域两端 */
        mp_size_t first = 0, last = 0;
        for (mp_size_t na = lo; na <= hi; ++na) {
            if (reach(na, nb)) {
                if (first == 0)
                    first = na;
                last = na;
            }
        }
        if (first == 0)
            continue;
        const mp_size_t nas[] = {first, first + 1, (first + last) / 2, last - 1, last};
        for (size_t i = 0; i < ARRAY_SIZE(nas); ++i) {
            mp_size_t na = nas[i];
            if (na < first || na > last || !reach(na, nb))
                continue;
            if (i > 0 && na == nas[i - 1])
                continue;
            check_kernel(fn, na, nb, &seed, tag);
            ++checked;
        }
    }
    TEST_CHECK_MSG(checked > 40, tag);
}

/* 带边界参数 = 入口断言保守比例式的整数化（偏移在除法前，lo/hi 含端点）：
   22: nb<=na<=2nb-5          32: nb+4<=na<=3nb-14
   33: nb<=na<=(3nb-16)/2     42: (3nb+10)/2<=na<=4nb-8（余类顶 4nb-23..4nb-8）
   43: nb+3<=na<=2nb-5        44: nb<=na<=(4nb-13)/3
   52: 2nb+3<=na<=5nb-9       53: (4nb+13)/3<=na<=(5nb-13)/2
   62: (5nb+8)/2<=na<=6nb-11 */
TEST_CASE("mul/toom_contracts", toom22_bounds) {
    sweep("toom22 bounds", lmmp_mul_toom22_, reach22, 2, 1, 1, 0, 2, 1, -5, 0x22b0da7e00000001ull);
}
TEST_CASE("mul/toom_contracts", toom32_bounds) {
    sweep("toom32 bounds", lmmp_mul_toom32_, reach32, 4, 1, 1, 4, 3, 1, -14, 0x32b0da7e00000002ull);
}
TEST_CASE("mul/toom_contracts", toom33_bounds) {
    sweep("toom33 bounds", lmmp_mul_toom33_, reach33, 26, 1, 1, 0, 3, 2, -16, 0x33b0da7e00000003ull);
}
TEST_CASE("mul/toom_contracts", toom42_bounds) {
    u64 seed = 0x42b0da7e00000004ull;
    /* 上沿余类精确式：扫描顶取 4nb-8（na mod 4==0 类），各余类真顶在
       4nb-23..4nb-8，由 reach 过滤后的集合与契约域一致 */
    sweep("toom42 bounds", lmmp_mul_toom42_, reach42, 20, 3, 2, 10, 4, 1, -8, seed);
    /* 四余类顶点逐一直调 + 42u 切换缝形状（旧保守上界与 3nb 分派界之间的
       真空带形状，余类精确式下合法）直调与经分派双验 */
    int checked = 0;
    for (mp_size_t nb = 20; nb <= 37; ++nb) {
        mp_size_t top[4] = {0, 0, 0, 0};
        for (mp_size_t na = (3 * nb + 10) / 2; na <= 4 * nb - 8; ++na)
            if (reach42(na, nb))
                top[na & 3] = na;
        for (int r = 0; r < 4; ++r)
            if (top[r]) {
                check_kernel(lmmp_mul_toom42_, top[r], nb, &seed, "toom42 residue top");
                ++checked;
            }
    }
    const mp_size_t seam[][2] = {
        {57, 20}, {58, 20}, {59, 20}, {61, 21}, {62, 21},
    };
    for (size_t i = 0; i < ARRAY_SIZE(seam); ++i) {
        check_kernel(lmmp_mul_toom42_, seam[i][0], seam[i][1], &seed, "toom42 seam");
        check_dispatch(seam[i][0], seam[i][1], &seed, "toom42 seam dispatch");
        checked += 2;
    }
    TEST_CHECK_MSG(checked > 60, "toom42 余类顶点覆盖数不足");
}
TEST_CASE("mul/toom_contracts", toom43_bounds) {
    sweep("toom43 bounds", lmmp_mul_toom43_, reach43, 14, 1, 1, 3, 2, 1, -5, 0x43b0da7e00000005ull);
}
TEST_CASE("mul/toom_contracts", toom44_bounds) {
    sweep("toom44 bounds", lmmp_mul_toom44_, reach44, 26, 1, 1, 0, 4, 3, -13, 0x44b0da7e00000006ull);
}
TEST_CASE("mul/toom_contracts", toom52_bounds) {
    sweep("toom52 bounds", lmmp_mul_toom52_, reach52, 9, 2, 1, 3, 5, 1, -9, 0x52b0da7e00000007ull);
}
TEST_CASE("mul/toom_contracts", toom53_bounds) {
    sweep("toom53 bounds", lmmp_mul_toom53_, reach53, 10, 4, 3, 13, 5, 2, -13, 0x53b0da7e00000008ull);
}
TEST_CASE("mul/toom_contracts", toom62_bounds) {
    sweep("toom62 bounds", lmmp_mul_toom62_, reach62, 9, 5, 2, 8, 6, 1, -11, 0x62b0da7e00000009ull);
}

/* 6h/8h：平衡端 + 阶梯各档代表比例 + 上沿（6h 至 3nb-9 闭、8h 至 4nb 闭），
   全部经切分谓词过滤（小 nb 阶梯切换洞不直调） */
TEST_CASE("mul/toom_contracts", toom6h_bounds) {
    u64 seed = 0x6ab0da7e0000000bull;
    int checked = 0;
    /* nb>=46 契约门起；上沿 na+9<=3nb（贴 3nb 的取整缝内 (9,4) 阶梯恢复后
       短侧段数降 2、±1 求值无路径，契约自 3nb 收至 3nb-9） */
    const mp_size_t nbs[] = {46, 47, 50, 56, 64, 80, 100, 150, 200};
    const double ratios[] = {1.0, 1.06, 1.2, 1.4, 1.6, 1.9, 2.2, 2.6, 2.9};
    for (size_t bi = 0; bi < ARRAY_SIZE(nbs); ++bi) {
        mp_size_t nb = nbs[bi];
        for (size_t ri = 0; ri < ARRAY_SIZE(ratios); ++ri) {
            mp_size_t na = (mp_size_t)(nb * ratios[ri]);
            if (na < nb || na + 9 > 3 * nb || !reach6h(na, nb))
                continue;
            check_kernel(lmmp_mul_toom6h_, na, nb, &seed, "toom6h bounds");
            ++checked;
        }
        /* 上沿：契约上沿 na<=3nb-9 内最大结构合法 na */
        for (mp_size_t na = 3 * nb - 9; na > nb; --na) {
            if (reach6h(na, nb)) {
                check_kernel(lmmp_mul_toom6h_, na, nb, &seed, "toom6h upper edge");
                ++checked;
                break;
            }
        }
    }
    TEST_CHECK_MSG(checked > 60, "toom6h 覆盖数不足");
}

TEST_CASE("mul/toom_contracts", toom8h_bounds) {
    u64 seed = 0x8ab0da7e0000000cull;
    int checked = 0;
    const mp_size_t nbs[] = {79, 80, 85, 90, 96, 110, 130, 160, 200, 305};
    const double ratios[] = {1.0, 1.1, 1.3, 1.6, 2.0, 2.4, 2.9, 3.4, 3.9};
    for (size_t bi = 0; bi < ARRAY_SIZE(nbs); ++bi) {
        mp_size_t nb = nbs[bi];
        for (size_t ri = 0; ri < ARRAY_SIZE(ratios); ++ri) {
            mp_size_t na = (mp_size_t)(nb * ratios[ri]);
            if (na < nb || na > 4 * nb || !reach8h(na, nb))
                continue;
            check_kernel(lmmp_mul_toom8h_, na, nb, &seed, "toom8h bounds");
            ++checked;
        }
        /* 上沿闭边界 na=4nb（大 nb 恒合法；小 nb 过滤） */
        if (reach8h(4 * nb, nb)) {
            check_kernel(lmmp_mul_toom8h_, 4 * nb, nb, &seed, "toom8h upper 4nb");
            ++checked;
        }
    }
    TEST_CHECK_MSG(checked > 80, "toom8h 覆盖数不足");
}

/* 三外层：首块尺寸门起逐 nb，na 下沿闭边界、下沿+1、双块、多块+尾块 */
TEST_CASE("mul/toom_contracts", unbalance_bounds) {
    u64 seed = 0x0ab0da7e0000000dull;
    int checked = 0;
    struct {
        toom_fn fn;
        mp_size_t gate;  /* nb 连续全绿门 */
        mp_size_t ratio; /* na 下沿 = ratio*nb */
        const char* tag;
    } cases[] = {
        {lmmp_mul_toom42_unbalance_, 8, 3, "toom42u bounds"},
        {lmmp_mul_toom62_unbalance_, 6, 5, "toom62u bounds"},
        {lmmp_mul_toom93_unbalance_, 17, 3, "toom93u bounds"},
    };
    for (size_t ci = 0; ci < ARRAY_SIZE(cases); ++ci) {
        for (mp_size_t nb = cases[ci].gate; nb <= cases[ci].gate + 15; ++nb) {
            mp_size_t r = cases[ci].ratio;
            const mp_size_t nas[] = {r * nb, r * nb + 1, (r + 2) * nb, (2 * r + 1) * nb + nb / 2};
            for (size_t i = 0; i < ARRAY_SIZE(nas); ++i) {
                check_kernel(cases[ci].fn, nas[i], nb, &seed, cases[ci].tag);
                ++checked;
            }
        }
    }
    TEST_CHECK_MSG(checked > 180, "unbalance 覆盖数不足");
}

/* 尺寸门-1 与小洞形状：入口断言拒绝，只走完整分派验证路由正确 */
TEST_CASE("mul/toom_contracts", below_gate_dispatch) {
    u64 seed = 0x9a7eb0da7e00000eull;
    const mp_size_t shapes[][2] = {
        {2, 1},   /* 22 门-1 */
        {5, 3},   /* 32 门-1 */
        {25, 25}, /* 33 门-1 */
        {4, 4},   /* 33 na=4 洞 */
        {30, 19}, /* 42 门-1 */
        {10, 7},  /* 43 门-1 */
        {11, 9},  /* 43 近平衡洞（na=nb+2） */
        {25, 25}, /* 44 门-1 */
        {9, 9},   /* 44 na=9 小洞 */
        {6, 6},   /* 44 na=6 小洞 */
        {5, 5},   /* 44 na=5 小洞 */
        {9, 4},   /* 52 门-1 */
        {4, 2},   /* 53 门-1 */
        {3, 1},   /* 62 门-1 */
        {90, 45},  /* 6h 门-1 */
        {230, 78}, /* 8h 门-1 */
        {20, 6},  /* 93 门-1 */
        {20, 7},  /* 42u 门-1 */
        {24, 5},  /* 62u 门-1 */
        {47, 16}, /* 93u 门-1 */
    };
    for (size_t i = 0; i < ARRAY_SIZE(shapes); ++i)
        check_dispatch(shapes[i][0], shapes[i][1], &seed, "below gate dispatch");
}

/* 大尺寸比例带内部直调（远离取整洞）+ 洞区经分派 */
TEST_CASE("mul/toom_contracts", large_band_edges) {
    u64 seed = 0x1a6eb0da7e00000full;
    int checked = 0;
    const mp_size_t nbs[] = {60, 100, 150, 200, 306};
    for (size_t bi = 0; bi < ARRAY_SIZE(nbs); ++bi) {
        mp_size_t nb = nbs[bi];
        /* 52 带内部：na=3nb-1（called 带上沿内） */
        check_kernel(lmmp_mul_toom52_, 3 * nb - 1, nb, &seed, "toom52 large interior");
        /* 62 带内部：na=4nb */
        check_kernel(lmmp_mul_toom62_, 4 * nb, nb, &seed, "toom62 large interior");
        /* 42 下沿正例：na=3nb/2+5（nb 偶时 2na=3nb+10、奇时 3nb+9 恰过缝） */
        check_kernel(lmmp_mul_toom42_, 3 * nb / 2 + 5, nb, &seed, "toom42 lower edge");
        /* 52/62 上沿洞区与 42 下沿反例：走分派 */
        check_dispatch(5 * nb - 1, nb, &seed, "toom52 hole dispatch");
        check_dispatch(6 * nb - 1, nb, &seed, "toom62 hole dispatch");
        check_dispatch(3 * nb / 2, nb, &seed, "toom42 below lower dispatch");
        checked += 6;
    }
    TEST_CHECK_MSG(checked >= 30, "large band 覆盖数不足");
}

void test_mul_toom_contracts(void) {
    TEST_RUN("mul/toom_contracts", toom22_bounds);
    TEST_RUN("mul/toom_contracts", toom32_bounds);
    TEST_RUN("mul/toom_contracts", toom33_bounds);
    TEST_RUN("mul/toom_contracts", toom42_bounds);
    TEST_RUN("mul/toom_contracts", toom43_bounds);
    TEST_RUN("mul/toom_contracts", toom44_bounds);
    TEST_RUN("mul/toom_contracts", toom52_bounds);
    TEST_RUN("mul/toom_contracts", toom53_bounds);
    TEST_RUN("mul/toom_contracts", toom62_bounds);
    TEST_RUN("mul/toom_contracts", toom6h_bounds);
    TEST_RUN("mul/toom_contracts", toom8h_bounds);
    TEST_RUN("mul/toom_contracts", unbalance_bounds);
    TEST_RUN("mul/toom_contracts", below_gate_dispatch);
    TEST_RUN("mul/toom_contracts", large_band_edges);
}

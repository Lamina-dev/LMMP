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

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

/*
    gcdext 验证（铁律：无自建算法 oracle，库除法欧几里得参考已删）：
      1. 贝祖恒等式 |s|*u ± |t|*v == g（32 位素数模同余）与极小系数界
         2*|s|*g <= v（库乘重建）
      2. g 值与 gcd_lehmer_ / gcd_2 独立路径交叉（归约序列不同）
      3. 写死构造：v|u → s=0、g=v；相邻 Fibonacci → g=1；u-1 → g=1
      4. 标量入口（gcdext_11）：64 位 % 欧几里得标量参考 + 穷举小值
*/

namespace {

u64 gcd_u64(u64 a, u64 b) {
    while (b) {
        u64 t = a % b;
        a = b;
        b = t;
    }
    return a;
}

// 有符号系数的绝对值写入 [out]（sn 为 mp_ssize_t，负表示负系数）
mp_size_t abs_coeffs(mp_ptr out, mp_ptr s, mp_ssize_t sn) {
    mp_size_t l = sn < 0 ? (mp_size_t)-sn : (mp_size_t)sn;
    memcpy(out, s, (size_t)l * 8);
    return l;
}

// 贝祖恒等式（模同余）：|s|*u ± |t|*v ≡ g (mod p)（符号按 sn/tn；同负取负）
bool bezout_identity_l(const mp_limb_t* u, mp_size_t un, const mp_limb_t* v, mp_size_t vn,
                       const mp_limb_t* g, mp_size_t gn, mp_ptr s, mp_ssize_t sn, mp_ptr t,
                       mp_ssize_t tn) {
    mp_size_t cap = un + vn + 8;
    mp_ptr sa = alloc_limbs(cap), ta = alloc_limbs(cap);
    mp_size_t ls = abs_coeffs(sa, s, sn), lt = abs_coeffs(ta, t, tn);
    bool ok = true;
    for (int i = 0; i < MOD_P_CNT && ok; ++i) {
        u64 p = MOD_P[i];
        u64 ps = ls ? mulmod32(mod32(sa, ls, (u32)p), mod32(u, un, (u32)p), p) : 0;
        u64 pt = lt ? mulmod32(mod32(ta, lt, (u32)p), mod32(v, vn, (u32)p), p) : 0;
        u64 lhs = (sn < 0) == (tn < 0) ? addmod32(ps, pt, p)
                                      : (sn < 0) ? submod32(pt, ps, p) : submod32(ps, pt, p);
        if ((sn < 0) && (tn < 0)) lhs = (p - lhs) % p;
        ok = lhs == mod32(g, gn, (u32)p);
    }
    lmmp_free(sa);
    lmmp_free(ta);
    return ok;
}

// 极小系数界：2*|s|*g <= v（库乘重建）
bool coeff_bound_l(const mp_limb_t* v, mp_size_t vn, const mp_limb_t* g, mp_size_t gn, mp_ptr s,
                   mp_ssize_t sn) {
    mp_size_t cap = vn + gn + 4;
    mp_ptr sa = alloc_limbs(cap), x = alloc_limbs(cap);
    mp_size_t ls = abs_coeffs(sa, s, sn);
    if (ls == 0) {
        lmmp_free(sa);
        lmmp_free(x);
        return true;
    }
    if (ls >= gn) lmmp_mul_(x, sa, ls, g, gn);
    else lmmp_mul_(x, g, gn, sa, ls);
    mp_size_t lx = ls + gn;
    lmmp_zero(x + lx, cap - lx);
    lmmp_add_n_(x, x, x, cap);  // x *= 2
    bool ok = mpn_cmp(x, cap, v, vn) <= 0;
    lmmp_free(sa);
    lmmp_free(x);
    return ok;
}

// 标量贝祖：模同余检查（s*u + t*v ≡ g (mod p)，32 位素数集）
bool scalar_bezout(u64 u, u64 v, u64 g, slong s, slong t) {
    u64 sa = s < 0 ? (u64)-s : (u64)s;
    u64 ta = t < 0 ? (u64)-t : (u64)t;
    for (int i = 0; i < MOD_P_CNT; ++i) {
        u64 p = MOD_P[i];
        u64 ps = (sa % p) * (u % p) % p;
        u64 pt = (ta % p) * (v % p) % p;
        u64 lhs = (s < 0) == (t < 0) ? (ps + pt) % p
                                   : (s < 0) ? submod32(pt, ps, p) : submod32(ps, pt, p);
        // 同负号时 s*u+t*v = -(ps+pt)，须 ≡ g
        if ((s < 0) && (t < 0)) lhs = (p - lhs) % p;
        if (lhs != g % p) return false;
    }
    return true;
}

// 汇总检查：恒等式 + 界（g 值由调用方另行验证）
bool check_gcdext(mp_ptr g, mp_size_t gn, mp_ptr s, mp_ssize_t sn, mp_ptr t, mp_ssize_t tn,
                  mp_srcptr u, mp_size_t un, mp_srcptr v, mp_size_t vn) {
    if (!bezout_identity_l(u, un, v, vn, g, gn, s, sn, t, tn)) return false;
    if (!coeff_bound_l(v, vn, g, gn, s, sn)) return false;
    if (!(mpn_eq(u, un, v, vn)) && !coeff_bound_l(u, un, g, gn, t, tn))
        return false;  // u==v 时 t=1 越界（定理前提 u!=v）
    return true;
}

}  // namespace

TEST_CASE("numth/gcdext", gcdext_11) {
    // 穷举小值：恒等式 + 界 + 标量参考
    for (u64 u = 1; u <= 300; ++u) {
        for (u64 v = 1; v <= 300; ++v) {
            slong s, t;
            ulong g = lmmp_gcdext_11_(u, v, &s, &t);
            if (g != gcd_u64(u, v)) TEST_CHECK_MSG(false, "gcd mismatch");
            TEST_CHECK_MSG(scalar_bezout(u, v, g, s, t), "bezout identity");
            u64 sa = s < 0 ? (u64)-s : (u64)s;
            u64 ta = t < 0 ? (u64)-t : (u64)t;
            TEST_CHECK_MSG(2 * sa * g <= v, "s bound");
            if (u != v) TEST_CHECK_MSG(2 * ta * g <= u, "t bound");
        }
    }
    // 随机 64 位
    u64 seed = 0x6d2b79f5ull;
    for (int i = 0; i < 20000; ++i) {
        u64 u = xorshift64(seed), v = xorshift64(seed);
        if (u == 0) u = 1;
        if (v == 0) v = 1;
        slong s, t;
        ulong g = lmmp_gcdext_11_(u, v, &s, &t);
        TEST_CHECK_MSG(scalar_bezout(u, v, g, s, t), "bezout identity random");
        TEST_CHECK_MSG(g == gcd_u64(u, v), "gcd random");
        u64 sa = s < 0 ? (u64)-s : (u64)s;
        TEST_CHECK_MSG(sa <= v / g / 2, "s bound random");  // 2*sa*g<=v 的无溢出形式
    }
    // 确定性边界：相等、整除、极值、t=NULL
    slong s, t;
    TEST_CHECK_EQ(lmmp_gcdext_11_(7, 7, &s, &t), 7u);
    TEST_CHECK(s == 0 && t == 1);
    TEST_CHECK_EQ(lmmp_gcdext_11_(UINT64_MAX, UINT64_MAX, &s, &t), UINT64_MAX);
    TEST_CHECK(s == 0 && t == 1);
    TEST_CHECK_EQ(lmmp_gcdext_11_(UINT64_MAX, 3, &s, &t), 3u);  // 3 | 2^64-1
    TEST_CHECK(scalar_bezout(UINT64_MAX, 3, 3, s, t));
    TEST_CHECK_EQ(lmmp_gcdext_11_(1, UINT64_MAX, &s, &t), 1u);
    TEST_CHECK(scalar_bezout(1, UINT64_MAX, 1, s, t));
    lmmp_gcdext_11_(12345, 6789, &s, NULL);  // t=NULL 路径
}

TEST_CASE("numth/gcdext", gcdext_1) {
    u64 seed = 0x3c6ef372fe94f82bull;
    for (mp_size_t un = 1; un <= 24; ++un) {
        for (int iter = 0; iter < 12; ++iter) {
            mp_ptr u = alloc_limbs(un);
            random_limbs(u, un, seed);
            u64 v = xorshift64(seed) | 1;
            mp_ptr g = alloc_limbs(1), s = alloc_limbs(1), t = alloc_limbs(un + 1);
            mp_ssize_t sn, tn;
            mp_size_t gn = lmmp_gcdext_1_(g, s, &sn, t, &tn, u, un, v);
            TEST_CHECK_MSG(gn == 1, "g length");
            // g 值标量参考：gcd(u, v) == gcd(u mod v, v)
            TEST_CHECK_MSG(g[0] == gcd_u64(lmmp_mod_1_(u, un, v), v), "g value");
            TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, u, un, &v, 1), "identity");
            // dstt=NULL
            mp_ssize_t sn2;
            mp_size_t gn2 = lmmp_gcdext_1_(g, s, &sn2, NULL, NULL, u, un, v);
            TEST_CHECK_MSG(gn2 == 1 && sn2 == sn, "null t keeps s");
            lmmp_free(u); lmmp_free(g); lmmp_free(s); lmmp_free(t);
        }
    }
    // dstg 与 up 别名（eqsep 契约）
    mp_ptr u = alloc_limbs(5), g = alloc_limbs(5), s = alloc_limbs(1), t = alloc_limbs(6);
    random_limbs(u, 5, seed);
    mp_limb_t vp[1] = {12345};
    mp_ssize_t sn, tn;
    lmmp_gcdext_1_(g, s, &sn, t, &tn, u, 5, 12345);
    TEST_CHECK_MSG(check_gcdext(g, 1, s, sn, t, tn, u, 5, vp, 1), "gcdext_1 baseline identity");
    TEST_CHECK_MSG(g[0] == gcd_u64(lmmp_mod_1_(u, 5, 12345), 12345), "baseline");
    lmmp_copy(g, u, 5);
    lmmp_gcdext_1_(g, s, &sn, NULL, NULL, g, 5, 12345);
    TEST_CHECK_MSG(g[0] == gcd_u64(lmmp_mod_1_(u, 5, 12345), 12345), "alias dstg=up");
    lmmp_free(u); lmmp_free(g); lmmp_free(s); lmmp_free(t);
}

TEST_CASE("numth/gcdext", gcdext_2) {
    u64 seed = 0x9e3779b97f4a7c15ull;
    for (mp_size_t un = 3; un <= 30; ++un) {
        for (int iter = 0; iter < 8; ++iter) {
            mp_ptr u = alloc_limbs(un), v = alloc_limbs(2);
            random_limbs(u, un, seed);
            random_limbs(v, 2, seed);
            mp_ptr g = alloc_limbs(2), s = alloc_limbs(2), t = alloc_limbs(un + 1);
            mp_ssize_t sn, tn;
            mp_size_t gn = lmmp_gcdext_2_(g, s, &sn, t, &tn, u, un, v);
            // g 与库 gcd_2 一致（交叉）+ 恒等式 + 界
            mp_ptr g2 = alloc_limbs(2);
            mp_size_t gn2 = lmmp_gcd_2_(g2, u, un, v);
            TEST_CHECK_MSG(mpn_eq(g, gn, g2, gn2), "g matches gcd_2");
            TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, u, un, v, 2), "identity");
            lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s); lmmp_free(t); lmmp_free(g2);
        }
    }
    // v | u：s=0、t=1（u = k*v 经库 mul_1 构造）
    for (int iter = 0; iter < 10; ++iter) {
        mp_ptr v = alloc_limbs(2);
        random_limbs(v, 2, seed);
        u64 k = 3 + (xorshift64(seed) & 0xf);
        mp_ptr u = alloc_limbs(4);
        u64 hi = lmmp_mul_1_(u, v, 2, k);
        mp_size_t un = hi ? 3 : 2;
        if (hi) u[2] = hi;
        mp_ptr g = alloc_limbs(2), s = alloc_limbs(2);
        mp_ssize_t sn;
        mp_size_t gn = lmmp_gcdext_2_(g, s, &sn, NULL, NULL, u, un, v);
        TEST_CHECK_MSG(mpn_eq(g, gn, v, 2) && sn == 0, "exact multiple");
        lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s);
    }
}

namespace {

// 对一对 limb 数组跑 lmmp_gcdext_ 并全检查（允许 un/vn 任意次序）。
// v 的容量取 max(un,vn)：mode 2/4 会把 un limb 的 u 拷入 v（近邻构造）。
void run_gcdext_pair(u64& seed, mp_size_t un, mp_size_t vn, int mode) {
    mp_ptr u = alloc_limbs(un + 2), v = alloc_limbs((un > vn ? un : vn) + 2);
    if (mode == 0) {  // 随机
        random_limbs(u, un, seed);
        random_limbs(v, vn, seed);
    } else if (mode == 1) {  // u = v * k（整除：s=0、g=v；库 mul_1 构造）
        random_limbs(v, vn, seed);
        lmmp_zero(u, un + 2);
        u64 k = 3 + (xorshift64(seed) & 0xFF);
        u64 hi = lmmp_mul_1_(u, v, vn, k);
        un = hi ? vn + 1 : vn;
        if (hi) u[vn] = hi;
    } else if (mode == 2) {  // 近邻（v = u - small）
        random_limbs(u, un, seed);
        lmmp_copy(v, u, un);
        vn = un;
        sub_small_inplace(v, vn, 1 + (xorshift64(seed) & 0xFF));
        vn = norm_len(v, vn);
    } else if (mode == 4) {  // 近邻极值 v = u - 1：gcd 恒 1
        random_limbs(u, un, seed);
        if (u[0] == 0) u[0] = 2;  // 保证借位减 u-1 仍在 un limb 内
        lmmp_copy(v, u, un);
        vn = un;
        sub_small_inplace(v, vn, 1);
        vn = norm_len(v, vn);
    } else {  // gcd > 1：u = 2x、v = 2y（库 add_n 翻倍）
        random_limbs(u, un, seed);
        random_limbs(v, vn, seed);
        lmmp_zero(u + un, 1);
        lmmp_zero(v + vn, 1);
        lmmp_add_n_(u, u, u, un + 1);
        lmmp_add_n_(v, v, v, vn + 1);
        un = norm_len(u, un + 1);
        vn = norm_len(v, vn + 1);
    }
    mp_ptr g = alloc_limbs(un + vn);
    /* s 容量按两个方向的最大 vn 取（swapped 调用时 s/t 角色互换） */
    mp_size_t mx = (vn > un ? vn : un) + 1;
    mp_ptr s = alloc_limbs(mx), t = alloc_limbs(mx);
    mp_ssize_t sn, tn;
    mp_size_t gn = lmmp_gcdext_(g, s, &sn, t, &tn, u, un, v, vn);
    mp_ptr gsave = alloc_limbs(un + vn);  // 首次调用的 g（已验证），别名检查参照
    mp_size_t gns = gn;
    memcpy(gsave, g, (size_t)gn * 8);
    if (mode == 1) {
        TEST_CHECK_MSG(mpn_eq(g, gn, v, vn) && sn == 0, "exact multiple gcd/s");
    } else if (mode == 4) {
        TEST_CHECK_MSG(gn == 1 && g[0] == 1, "neighbor-1 gcd");
        TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, u, un, v, vn), "neighbor-1 identity");
    } else {
        // g 与 gcd_lehmer_ 独立路径交叉（gcdext_lehmer/gcdext_hgcd 的 g 计算
        // 与 gcd_lehmer 分属不同实现文件，归约序列不同）
        mp_ptr gr = alloc_limbs(un + vn);
        mp_size_t en = lmmp_gcd_lehmer_(gr, u, un, v, vn);
        TEST_CHECK_MSG(mpn_eq(g, gn, gr, en), "g vs gcd_lehmer_");
        lmmp_free(gr);
        TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, u, un, v, vn), "identity");
    }
    // 参数交换（s/t 角色互换）
    mp_ssize_t sn2, tn2;
    mp_size_t gn2 = lmmp_gcdext_(g, s, &sn2, t, &tn2, v, vn, u, un);
    (void)gn2;
    if (mode == 4) {
        TEST_CHECK_MSG(gn2 == 1 && g[0] == 1, "neighbor-1 swapped gcd");
    } else if (mode != 1) {
        TEST_CHECK_MSG(check_gcdext(g, gn2, s, sn2, t, tn2, v, vn, u, un), "swapped");
    }
    // dstt=NULL
    mp_ssize_t sn3;
    lmmp_gcdext_(g, s, &sn3, NULL, NULL, u, un, v, vn);
    TEST_CHECK_MSG(sn3 == sn, "null t keeps s");
    // dstg 与 up 别名（参照首次调用已验证的 g 值）
    mp_ptr u2 = alloc_limbs(un + vn);
    lmmp_copy(u2, u, un);
    lmmp_gcdext_(u2, s, &sn, NULL, NULL, u2, un, v, vn);
    TEST_CHECK_MSG(mpn_eq(u2, gn, gsave, gns), "alias dstg=up");
    lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s); lmmp_free(t);
    lmmp_free(u2);
    lmmp_free(gsave);
}

}  // namespace

TEST_CASE("numth/gcdext", gcdext_lehmer_and_dispatch) {
    u64 seed = 0x5ac635d8aa3a93e7ull;
    // 尺寸矩阵：跨 1/2/多 limb、GCD_HGCD_THRESHOLD=86 两侧、平衡与悬殊
    struct { mp_size_t un, vn; } sizes[] = {
        {1, 1},   {2, 1},   {2, 2},   {3, 1},   {3, 2},   {3, 3},   {5, 2},   {5, 4},
        {9, 3},   {9, 8},   {17, 5},  {17, 16}, {33, 7},  {33, 32}, {60, 3},  {60, 59},
        {85, 85}, {86, 43}, {87, 2},  {87, 86}, {120, 61}, {200, 3}, {200, 100},
    };  // 大尺寸 gcdext 由 gcd 三路径交叉 + 贝祖恒等式覆盖
    for (auto& sz : sizes) {
        int iters = sz.un > 60 ? 1 : 3;
        for (int mode = 0; mode < 5; ++mode) {
            for (int iter = 0; iter < iters; ++iter) {
                run_gcdext_pair(seed, sz.un, sz.vn, mode);
            }
        }
    }
}

TEST_CASE("numth/gcdext", gcdext_fibonacci) {
    // 相邻斐波那契（商全 1、系数最大形态）；fibonacci2_ 库构造相邻对
    // F(k), F(k-1)，gcd=1 为数学已知
    for (mp_size_t n : {3, 8, 20, 50, 90}) {
        ulong k = 3;
        for (;; ++k) {
            if (lmmp_fibonacci_size_(k) >= n) break;
        }
        mp_size_t rn = lmmp_fibonacci_size_(k);
        mp_ptr u = alloc_limbs(rn + 1), v = alloc_limbs(rn + 1);
        lmmp_zero(u, rn + 1);
        lmmp_zero(v, rn + 1);
        lmmp_fibonacci2_(u, v, rn, k);  // 契约：u=F(k), v=F(k-1)
        mp_size_t un = norm_len(u, rn + 1), vn = norm_len(v, rn + 1);
        mp_ptr g = alloc_limbs(rn), s = alloc_limbs(vn + 1), t = alloc_limbs(un + 1);
        mp_ssize_t sn, tn;
        mp_size_t gn = lmmp_gcdext_(g, s, &sn, t, &tn, u, un, v, vn);
        TEST_CHECK_MSG(gn == 1 && g[0] == 1, "fibonacci gcd 1");
        TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, u, un, v, vn), "fibonacci identity");
        lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s); lmmp_free(t);
    }
}

TEST_CASE("numth/gcdext", gcdext_large_cross) {
    // 大尺寸（gcdext 分发走 hgcd 分治）：贝祖恒等式 + 系数界 + g 与
    // gcd_lehmer_/gcd_ 双路径交叉（归约序列互异）
    u64 seed = 0x9e3779b97f4a7c15ull;
    struct { mp_size_t un, vn; } sizes[] = {{200, 100}, {300, 120}, {400, 399}};
    for (auto& sz : sizes) {
        mp_ptr u = alloc_limbs(sz.un), v = alloc_limbs(sz.vn);
        random_limbs(u, sz.un, seed);
        random_limbs(v, sz.vn, seed);
        v[sz.vn - 1] >>= 1;
        if (v[sz.vn - 1] == 0) v[sz.vn - 1] = 1;
        mp_ptr g = alloc_limbs(sz.vn), s = alloc_limbs(sz.vn + 2), t = alloc_limbs(sz.un + 2),
               gr = alloc_limbs(sz.vn), g2 = alloc_limbs(sz.vn);
        mp_ssize_t sn, tn;
        mp_size_t gn = lmmp_gcdext_(g, s, &sn, t, &tn, u, sz.un, v, sz.vn);
        mp_size_t en = lmmp_gcd_lehmer_(gr, u, sz.un, v, sz.vn);
        mp_size_t hn = lmmp_gcd_(g2, u, sz.un, v, sz.vn);
        TEST_CHECK_MSG(mpn_eq(g, gn, gr, en), "gcdext g vs lehmer");
        TEST_CHECK_MSG(mpn_eq(g, gn, g2, hn), "gcdext g vs gcd_");
        TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, u, sz.un, v, sz.vn), "bezout+bounds");
        lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s); lmmp_free(t);
        lmmp_free(gr); lmmp_free(g2);
    }
}

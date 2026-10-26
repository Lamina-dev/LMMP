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

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstdio>
#include <cstring>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed, bool msb = true) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0 && msb) p[n - 1] |= (u64)1 << 63;
}

// 标量恒等式：|s|*u ± |t|*v == g（符号相同取和，相异取大减小）
bool scalar_bezout(u64 u, u64 v, u64 g, slong s, slong t) {
    u64 sa = s < 0 ? (u64)-s : (u64)s;
    u64 ta = t < 0 ? (u64)-t : (u64)t;
    u128 p = (u128)sa * u, q = (u128)ta * v;
    if ((s < 0) == (t < 0))
        return p + q == (u128)g;
    u128 hi = p < q ? q : p, lo = p < q ? p : q;
    return lo + (u128)g == hi;
}

// 贝祖恒等式（定理级判据）：|s|*u ± |t|*v == g（符号相同取和，相异取大减小）
bool bezout_identity(const BigInt& u, const BigInt& v, const BigInt& g, const BigInt& s_abs,
                     bool s_neg, const BigInt& t_abs, bool t_neg) {
    BigInt p = BigInt::mul_school(s_abs, u);
    BigInt q = BigInt::mul_school(t_abs, v);
    if (s_neg == t_neg)
        return BigInt::add_abs(p, q) == g;
    const BigInt& hi = p < q ? q : p;
    const BigInt& lo = p < q ? p : q;
    return BigInt::add_abs(lo, g) == hi;
}

// 极小系数界：|s| <= v/(2g) 即 2*|s|*g <= v
bool coeff_bound(const BigInt& v, const BigInt& g, const BigInt& s_abs) {
    BigInt x = BigInt::mul_school(s_abs, g);
    x = BigInt::add_abs(x, x);
    return !(v < x);
}

// 汇总一次 gcdext 调用的全部检查（g 对照 + 恒等式 + 界）
bool check_gcdext(mp_ptr g, mp_size_t gn, mp_ptr s, mp_ssize_t sn, mp_ptr t, mp_ssize_t tn, const BigInt& bu,
                  const BigInt& bv) {
    BigInt bg(g, (size_t)gn);
    BigInt bref = BigInt::gcd_euclid(bu, bv);
    if (!(bg == bref))
        return false;
    BigInt s_abs(s, sn < 0 ? (size_t)-sn : (size_t)sn);
    BigInt t_abs(t, tn < 0 ? (size_t)-tn : (size_t)tn);
    if (!bezout_identity(bu, bv, bref, s_abs, sn < 0, t_abs, tn < 0)) {
        fprintf(stderr, "[chk] identity FAIL sn=%lld tn=%lld\n", (long long)sn, (long long)tn);
        return false;
    }
    if (!coeff_bound(bv, bref, s_abs)) {
        fprintf(stderr, "[chk] s-bound FAIL\n");
        return false;
    }
    if (!(bu == bv) && !coeff_bound(bu, bref, t_abs)) { // u==v 时 t=1 越界（定理前提 u!=v）
        fprintf(stderr, "[chk] t-bound FAIL\n");
        return false;
    }
    return true;
}

}  // namespace

TEST_CASE("numth/gcdext", gcdext_11) {
    // 穷举小值：恒等式 + 界（128 位幅度算术直接验证）
    for (u64 u = 1; u <= 300; ++u) {
        for (u64 v = 1; v <= 300; ++v) {
            slong s, t;
            ulong g = lmmp_gcdext_11_(u, v, &s, &t);
            if (g != lmmp_gcd_11_(u, v))
                TEST_CHECK_MSG(false, "gcd mismatch");
            TEST_CHECK_MSG(scalar_bezout(u, v, g, s, t), "bezout identity");
            // u==v 时 s=0、t=1，t 的极小界 |t|<=u/(2g) 退化为 1<=1/2 不成立（定理前提 u!=v）
            u64 sa = s < 0 ? (u64)-s : (u64)s;
            u64 ta = t < 0 ? (u64)-t : (u64)t;
            TEST_CHECK_MSG(2 * sa * g <= v, "s bound");
            if (u != v)
                TEST_CHECK_MSG(2 * ta * g <= u, "t bound");
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
        TEST_CHECK_MSG(g == lmmp_gcd_11_(u, v), "gcd random");
        u64 sa = s < 0 ? (u64)-s : (u64)s;
        u64 ta = t < 0 ? (u64)-t : (u64)t;
        TEST_CHECK_MSG((u128)2 * sa * g <= (u128)v, "s bound random");
        if (u != v)
            TEST_CHECK_MSG((u128)2 * ta * g <= (u128)u, "t bound random");
    }
    // 确定性边界：相等、整除、极值、t=NULL
    slong s, t;
    TEST_CHECK_EQ(lmmp_gcdext_11_(7, 7, &s, &t), 7u);
    TEST_CHECK(s == 0 && t == 1);
    TEST_CHECK_EQ(lmmp_gcdext_11_(UINT64_MAX, UINT64_MAX, &s, &t), UINT64_MAX);
    TEST_CHECK(s == 0 && t == 1);
    TEST_CHECK_EQ(lmmp_gcdext_11_(UINT64_MAX, 3, &s, &t), 3u); // 3 | 2^64-1
    TEST_CHECK(scalar_bezout(UINT64_MAX, 3, 3, s, t));
    TEST_CHECK_EQ(lmmp_gcdext_11_(1, UINT64_MAX, &s, &t), 1u);
    TEST_CHECK(scalar_bezout(1, UINT64_MAX, 1, s, t));
    lmmp_gcdext_11_(12345, 6789, &s, NULL); // t=NULL 路径
}

TEST_CASE("numth/gcdext", gcdext_1) {
    u64 seed = 0x3c6ef372fe94f82bull;
    for (mp_size_t un = 1; un <= 24; ++un) {
        for (int iter = 0; iter < 12; ++iter) {
            mp_ptr u = alloc_limbs(un);
            random_limbs(u, un, seed);
            u64 v = xorshift64(seed) | 1;
            BigInt bu(u, un), bv(v);
            mp_ptr g = alloc_limbs(1);
            mp_ptr s = alloc_limbs(1);
            mp_ptr t = alloc_limbs(un + 1);
            mp_ssize_t sn, tn;
            mp_size_t gn = lmmp_gcdext_1_(g, s, &sn, t, &tn, u, un, v);
            TEST_CHECK_MSG(gn == 1, "g length");
            TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, bu, bv), "identity");
            // dstt=NULL
            mp_ssize_t sn2;
            mp_size_t gn2 = lmmp_gcdext_1_(g, s, &sn2, NULL, NULL, u, un, v);
            TEST_CHECK_MSG(gn2 == 1 && sn2 == sn, "null t keeps s");
            lmmp_free(u); lmmp_free(g); lmmp_free(s); lmmp_free(t);
        }
    }
    // dstg 与 up 别名（eqsep 契约）：先用原始 u 建立基线，再以新副本做别名调用
    mp_ptr u = alloc_limbs(5);
    random_limbs(u, 5, seed);
    BigInt bu(u, 5);
    mp_ptr g = alloc_limbs(5);
    mp_ptr s = alloc_limbs(1);
    mp_ssize_t sn, tn;
    mp_ptr t = alloc_limbs(6);
    lmmp_gcdext_1_(g, s, &sn, t, &tn, u, 5, 12345);
    TEST_CHECK_MSG(check_gcdext(g, 1, s, sn, t, tn, bu, BigInt(12345)), "baseline");
    lmmp_copy(g, u, 5);
    lmmp_gcdext_1_(g, s, &sn, NULL, NULL, g, 5, 12345);
    TEST_CHECK_MSG(BigInt(g, (size_t)1) == BigInt::gcd_euclid(bu, BigInt(12345)), "alias dstg=up");
    lmmp_free(u); lmmp_free(g); lmmp_free(s); lmmp_free(t);
}

TEST_CASE("numth/gcdext", gcdext_2) {
    u64 seed = 0x9e3779b97f4a7c15ull;
    for (mp_size_t un = 3; un <= 30; ++un) {
        for (int iter = 0; iter < 8; ++iter) {
            mp_ptr u = alloc_limbs(un);
            mp_ptr v = alloc_limbs(2);
            random_limbs(u, un, seed);
            random_limbs(v, 2, seed);
            BigInt bu(u, un), bv(v, 2);
            mp_ptr g = alloc_limbs(2);
            mp_ptr s = alloc_limbs(2);
            mp_ptr t = alloc_limbs(un + 1);
            mp_ssize_t sn, tn;
            mp_size_t gn = lmmp_gcdext_2_(g, s, &sn, t, &tn, u, un, v);
            TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, bu, bv), "identity");
            lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s); lmmp_free(t);
        }
    }
    // v | u：s=0、t=1
    for (int iter = 0; iter < 10; ++iter) {
        mp_ptr v = alloc_limbs(2);
        random_limbs(v, 2, seed);
        BigInt bv(v, 2);
        BigInt bu = BigInt::mul_school(bv, BigInt(3 + (xorshift64(seed) & 0xf)));
        mp_size_t un = (mp_size_t)bu.d.size();
        mp_ptr u = alloc_limbs(un);
        to_limbs(bu, u, un);
        mp_ptr g = alloc_limbs(2);
        mp_ptr s = alloc_limbs(2);
        mp_ssize_t sn, tn;
        mp_size_t gn = lmmp_gcdext_2_(g, s, &sn, NULL, NULL, u, un, v);
        TEST_CHECK_MSG(BigInt(g, (size_t)gn) == bv && sn == 0, "exact multiple");
        lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s);
    }
}

namespace {

// 对一对 limb 数组跑 lmmp_gcdext_ 并全检查（允许 un/vn 任意次序）
void run_gcdext_pair(u64& seed, mp_size_t un, mp_size_t vn, int mode) {
    mp_ptr u = alloc_limbs(un);
    mp_ptr v = alloc_limbs(vn);
    if (mode == 0) {  // 随机
        random_limbs(u, un, seed);
        random_limbs(v, vn, seed);
    } else if (mode == 1) {  // u = v * k（随机标量倍数，整除：s=0、t=1）。
        // 不用 u = v*B^d 形状：BigInt oracle 的 divmod_school 在 (x<<s) 型输入上
        // 有已知疑似缺陷（见 test_numth_gcd.cpp 的 gcd128_ref 注），会卡死
        random_limbs(v, vn, seed);
        lmmp_free(u);
        BigInt bv0(v, vn);
        BigInt bu0 = BigInt::mul_school(bv0, BigInt(3 + (xorshift64(seed) & 0xFF)));
        un = (mp_size_t)bu0.d.size();
        u = alloc_limbs(un);
        to_limbs(bu0, u, un);
    } else if (mode == 2) {  // 近邻（v = u - small，顶端 63 位窗口相同，走除法回退）。
        // 避免位移型结构（oracle divmod_school 已知缺陷，见 mode 1 注）
        random_limbs(u, un, seed);
        BigInt bu2(u, un);
        BigInt bv2 = BigInt::sub_small(bu2, 1 + (xorshift64(seed) & 0xFF));
        lmmp_free(v);
        vn = (mp_size_t)bv2.d.size();
        v = alloc_limbs(vn);
        to_limbs(bv2, v, vn);
    } else if (mode == 4) {  // 近邻极值 v = u - 1：gcd 恒 1（驱动除法步/失衡守卫回归）
        random_limbs(u, un, seed);
        if (u[0] == 0)
            u[0] = 2; // 保证借位减 u-1 仍在 un limb 内
        BigInt bu2(u, un);
        BigInt bv2 = BigInt::sub_small(bu2, 1);
        lmmp_free(v);
        vn = (mp_size_t)bv2.d.size();
        v = alloc_limbs(vn);
        to_limbs(bv2, v, vn);
    } else {  // gcd > 1：u = 2x、v = 2y（x、y 随机）
        random_limbs(u, un, seed);
        random_limbs(v, vn, seed);
        BigInt x(u, un), y(v, vn);
        BigInt xu = BigInt::add_abs(x, x), yv = BigInt::add_abs(y, y);
        lmmp_free(u);
        lmmp_free(v);
        un = (mp_size_t)xu.d.size();
        vn = (mp_size_t)yv.d.size();
        u = alloc_limbs(un);
        v = alloc_limbs(vn);
        to_limbs(xu, u, un);
        to_limbs(yv, v, vn);
    }
    BigInt bu(u, un), bv(v, vn);
    mp_ptr g = alloc_limbs(un + vn);
    /* s 容量按两个方向的最大 vn 取（swapped 调用时 s/t 角色互换） */
    mp_ptr s = alloc_limbs((vn > un ? vn : un) + 1);
    mp_ptr t = alloc_limbs((vn > un ? vn : un) + 1);
    mp_ssize_t sn, tn;
    mp_size_t gn = lmmp_gcdext_(g, s, &sn, t, &tn, u, un, v, vn);
    if (mode == 1) {
        // 整除用例绕开 BigInt 欧几里得 oracle（divmod_school 在整除/移位型输入上的
        // 已知缺陷会卡死），直接对照已知答案 g = v、s = 0、t = 1
        TEST_CHECK_MSG(BigInt(g, (size_t)gn) == bv && sn == 0, "exact multiple gcd/s");
    } else if (mode == 4) {
        // 近邻极值已知答案：gcd(u, u-1) == 1，恒等式走轻量 BigInt 乘法检查
        TEST_CHECK_MSG(gn == 1 && g[0] == 1, "neighbor-1 gcd");
        BigInt s_abs(s, sn < 0 ? (size_t)-sn : (size_t)sn);
        BigInt t_abs(t, tn < 0 ? (size_t)-tn : (size_t)tn);
        TEST_CHECK_MSG(bezout_identity(bu, bv, BigInt(1), s_abs, sn < 0, t_abs, tn < 0),
                       "neighbor-1 identity");
    } else {
        TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, bu, bv), "identity");
    }
    // 参数交换（s/t 角色互换）
    mp_ssize_t sn2, tn2;
    mp_size_t gn2 = lmmp_gcdext_(g, s, &sn2, t, &tn2, v, vn, u, un);
    if (mode == 4) {
        TEST_CHECK_MSG(gn2 == 1 && g[0] == 1, "neighbor-1 swapped gcd");
    } else if (mode != 1) {
        TEST_CHECK_MSG(check_gcdext(g, gn2, s, sn2, t, tn2, bv, bu), "swapped");
    }
    // dstt=NULL
    mp_ssize_t sn3;
    lmmp_gcdext_(g, s, &sn3, NULL, NULL, u, un, v, vn);
    TEST_CHECK_MSG(sn3 == sn, "null t keeps s");
    // dstg 与 up 别名
    mp_ptr u2 = alloc_limbs(un);
    lmmp_copy(u2, u, un);
    lmmp_gcdext_(u2, s, &sn, NULL, NULL, u2, un, v, vn);
    if (mode == 4) {
        TEST_CHECK_MSG(BigInt(u2, (size_t)gn) == BigInt(1), "neighbor-1 alias dstg=up");
    } else if (mode != 1) {
        TEST_CHECK_MSG(BigInt(u2, (size_t)gn) == BigInt::gcd_euclid(bu, bv), "alias dstg=up");
    }
    lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s); lmmp_free(t); lmmp_free(u2);
}

}  // namespace

TEST_CASE("numth/gcdext", gcdext_lehmer_and_dispatch) {
    u64 seed = 0x5ac635d8aa3a93e7ull;
    // 尺寸矩阵：跨 1/2/多 limb、GCD_HGCD_THRESHOLD=86 两侧、平衡与悬殊
    struct { mp_size_t un, vn; } sizes[] = {
        {1, 1},   {2, 1},   {2, 2},   {3, 1},   {3, 2},   {3, 3},   {5, 2},   {5, 4},
        {9, 3},   {9, 8},   {17, 5},  {17, 16}, {33, 7},  {33, 32}, {60, 3},  {60, 59},
        {85, 85}, {86, 43}, {87, 2},  {87, 86}, {120, 61}, {200, 3}, {200, 100},
        {260, 130}, {300, 282}, {400, 40},
    };
    for (auto& sz : sizes) {
        int iters = sz.un > 60 ? 1 : 3; // 大尺寸 BigInt 欧几里得 oracle 代价高，减采样
        for (int mode = 0; mode < 5; ++mode) {
            if (sz.un > 60 && mode == 2)
                continue; // 近邻模式在 oracle 上退化为长欧几里得链，大尺寸跳过
            for (int iter = 0; iter < iters; ++iter) {
                run_gcdext_pair(seed, sz.un, sz.vn, mode);
            }
        }
    }
}

TEST_CASE("numth/gcdext", gcdext_fibonacci) {
    // 相邻斐波那契（商全 1、系数最大形态）
    u64 seed = 0x2545f4914f6cdd1dull;
    for (mp_size_t n : {3, 8, 20, 50, 90}) {
        BigInt x(1), y(1);
        while ((mp_size_t)y.d.size() < n || (mp_size_t)x.d.size() < n) {
            BigInt t = BigInt::add_abs(x, y);
            x = y;
            y = t;
        }
        if (x < y) {
            BigInt t = x; x = y; y = t;
        }
        mp_size_t un = (mp_size_t)x.d.size(), vn = (mp_size_t)y.d.size();
        mp_ptr u = alloc_limbs(un), v = alloc_limbs(vn);
        to_limbs(x, u, un);
        to_limbs(y, v, vn);
        mp_ptr g = alloc_limbs(vn);
        mp_ptr s = alloc_limbs(vn + 1);
        mp_ptr t = alloc_limbs(un + 1);
        mp_ssize_t sn, tn;
        mp_size_t gn = lmmp_gcdext_(g, s, &sn, t, &tn, u, un, v, vn);
        TEST_CHECK_MSG(check_gcdext(g, gn, s, sn, t, tn, x, y), "fibonacci identity");
        lmmp_free(u); lmmp_free(v); lmmp_free(g); lmmp_free(s); lmmp_free(t);
    }
}

TEST_CASE("numth/gcdext", modinv_ulong) {
    // 穷举小值
    for (u64 m = 1; m <= 200; ++m) {
        for (u64 u = 1; u <= 200; ++u) {
            ulong inv = lmmp_modinv_ulong_(u, m);
            if (m == 1) {
                TEST_CHECK_MSG(inv == 0, "m=1 inverse is 0");
                continue;
            }
            u64 r = u % m;
            ulong g = (r == 0) ? m : lmmp_gcd_11_(r, m);
            if (g != 1) {
                TEST_CHECK_MSG(inv == 0, "non-invertible");
            } else {
                TEST_CHECK_MSG((u128)u * inv % (u128)m == 1 && inv < m, "inverse");
            }
        }
    }
    // 随机 64 位
    u64 seed = 0x51ce1b7ea4df7c21ull;
    for (int i = 0; i < 20000; ++i) {
        u64 m = xorshift64(seed);
        u64 u = xorshift64(seed);
        if (m == 0) m = 1;
        if (u == 0) u = 1;
        ulong inv = lmmp_modinv_ulong_(u, m);
        u64 r = u % m;
        ulong g = (r == 0) ? m : lmmp_gcd_11_(r, m);
        if (m == 1) {
            TEST_CHECK_MSG(inv == 0, "m=1");
        } else if (g != 1) {
            TEST_CHECK_MSG(inv == 0, "non-invertible random");
        } else {
            u128 prod = (u128)u * inv;
            TEST_CHECK_MSG(prod % (u128)m == 1 && inv < m, "inverse random");
        }
    }
}

TEST_CASE("numth/gcdext", modinv_big) {
    u64 seed = 0x2b44ae9d4d6c8f13ull;
    for (mp_size_t mn : {2, 3, 5, 9, 17, 33, 60}) {
        for (int iter = 0; iter < 7; ++iter) {
            // 构造互素对：m 随机，u = m + 1 的倍数调整保证 gcd(u,m)=1 时逆存在
            mp_ptr m = alloc_limbs(mn);
            random_limbs(m, mn, seed);
            m[0] |= 1;
            BigInt bm(m, mn);
            BigInt bu;
            mp_ptr u;
            mp_size_t un;
            if (iter == 6) {
                // un < mn：u 直接取短随机数（lmmp_div_ 要求 na>=nb，覆盖 modinv 短分支）
                un = mn / 2 > 0 ? mn / 2 : 1;
                u = alloc_limbs(un);
                random_limbs(u, un, seed);
                u[0] |= 1;
                bu = BigInt(u, un);
            } else if (iter % 2 == 0) {
                // u = m + 1（与 m 互素）
                bu = BigInt::add_small(bm, 1);
                un = (mp_size_t)bu.d.size();
                u = alloc_limbs(un);
                to_limbs(bu, u, un);
            } else {
                un = mn;
                u = alloc_limbs(un);
                random_limbs(u, un, seed);
                u[0] |= 1;
                bu = BigInt(u, un);
            }
            mp_ptr dst = alloc_limbs(mn);
            mp_size_t rn = lmmp_modinv_(dst, u, un, m, mn);
            BigInt gcdr = BigInt::gcd_euclid(bu, bm);
            if (gcdr == BigInt(1)) {
                TEST_CHECK_MSG(rn > 0, "inverse exists");
                BigInt prod = BigInt::mul_school(BigInt(dst, (size_t)rn), bu);
                BigInt q, r;
                q = BigInt::divmod_school(prod, bm, r);
                TEST_CHECK_MSG(r == BigInt(1), "inv*u mod m == 1");
                TEST_CHECK_MSG(BigInt(dst, (size_t)rn) < bm, "inverse in range");
            } else {
                TEST_CHECK_MSG(rn == 0, "non-invertible reported");
            }
            // dst 与 u 别名（eqsep 契约；dst 容量按契约取 mn，un<mn 时大于 un）
            mp_ptr u2 = alloc_limbs(un > mn ? un : mn);
            lmmp_copy(u2, u, un);
            mp_size_t rn2 = lmmp_modinv_(u2, u2, un, m, mn);
            TEST_CHECK_MSG(rn2 == rn, "alias stable");
            lmmp_free(m); lmmp_free(u); lmmp_free(dst); lmmp_free(u2);
        }
    }
}

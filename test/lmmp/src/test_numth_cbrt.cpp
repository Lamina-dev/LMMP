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

/* cbrt 家族验证（无 BigInt oracle；sqrt 见 test_numth_sqrt.cpp）：
   1. floor 恒等式：r³ ≤ n < (r+1)³（(r+1)^3 = r³+3r²+3r+1 经库乘加重建）、
      余数恒等式 n == r³ + rem
   2. 标量档（u64）：icbrt 二分参考（64 位内）+ 完全立方角点
   3. 入口互比：cbrt_3/cbrt_6/cbrt_divide_/cbrt_ 生产入口；完全立方构造
      （sqr+mul 后 ±1）命中精确语义
   4. 逆立方根语义：invcbrt 下近似区间 [I-1, I]（参照经 div_+cbrt_ 库链构造）
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

// u64 isqrt 简单参考（二分，64 位内；root=2 交叉用）
u64 isqrt_u64(u64 x) {
    if (x == 0) return 0;
    u64 lo = 1, hi = 0xffffffffull;
    while (lo < hi) {
        u64 mid = lo + (hi - lo + 1) / 2;
        if (mid <= x / mid) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

u64 icbrt_u64(u64 x) {
    if (x == 0) return 0;
    u64 lo = 1, hi = 2642246ull;  // floor(cbrt(2^64-1))
    while (lo < hi) {
        u64 mid = lo + (hi - lo + 1) / 2;
        // mid*mid*mid <= x：mid <= 2642246 ⟹ 三积 < 2^64
        if (mid <= x / (mid * mid)) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

// r³ ≤ n < (r+1)³：立方经库乘两步 + 多项式展开 (r+1)³ = r³ + 3r² + 3r + 1
bool cbrt_floor_ok(const mp_limb_t* r, mp_size_t rn, const mp_limb_t* n, mp_size_t na) {
    mp_size_t mx = 3 * rn + 4;
    mp_ptr t1 = alloc_limbs(2 * rn), t2 = alloc_limbs(3 * rn), t3 = alloc_limbs(mx),
           add = alloc_limbs(mx), r2x3 = alloc_limbs(2 * rn + 1), r3x = alloc_limbs(rn + 1);
    lmmp_sqr_(t1, r, rn);              // r^2
    lmmp_mul_(t2, t1, 2 * rn, r, rn);  // r^3
    lmmp_zero(t3, mx);
    memcpy(t3, t2, (size_t)(3 * rn) * 8);
    u64 hi = lmmp_mul_1_(r2x3, t1, 2 * rn, 3);  // 3r^2
    mp_size_t l = 2 * rn;
    if (hi) r2x3[l++] = hi;
    lmmp_zero(add, mx);
    memcpy(add, r2x3, (size_t)l * 8);
    lmmp_add_n_(t3, t3, add, mx);
    lmmp_zero(r3x, rn + 1);  // 3r + 1
    memcpy(r3x, r, (size_t)rn * 8);
    lmmp_mul_1_(r3x, r3x, rn + 1, 3);
    add_small_inplace(r3x, rn + 1, 1);
    lmmp_zero(add, mx);
    memcpy(add, r3x, (size_t)(rn + 1) * 8);
    lmmp_add_n_(t3, t3, add, mx);
    mp_size_t np1 = norm_len(t3, mx);
    bool ok = mpn_cmp(t2, 3 * rn, n, na) <= 0 && mpn_cmp(n, na, t3, np1) < 0;
    lmmp_free(t1); lmmp_free(t2); lmmp_free(t3); lmmp_free(add); lmmp_free(r2x3); lmmp_free(r3x);
    return ok;
}

}  // namespace

TEST_CASE("numth/cbrt", cbrt_scalar_and_cbrt_3) {
    u64 seed = 0x0badcafef00dfaceull;
    for (int i = 0; i < 1000; ++i) {
        u64 x = xorshift64(seed);
        if (x == 0) x = 1;
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(x), icbrt_u64(x));
        TEST_CHECK_EQ(lmmp_cbrt_chebyshev_(x), icbrt_u64(x));
        TEST_CHECK_EQ(lmmp_nthroot_ulong_(x, 3), icbrt_u64(x));
        TEST_CHECK_EQ(lmmp_nthroot_ulong_(x, 2), isqrt_u64(x));  // root=2 分派 sqrt_ulong_
    }
    TEST_CHECK_EQ(lmmp_cbrt_ulong_(1), 1u);
    TEST_CHECK_EQ(lmmp_cbrt_ulong_(UINT64_MAX), 2642245ull);  // floor(cbrt(2^64-1))
    // 完全立方角点（u64 内）：cbrt(r³)=r、cbrt(r³-1)=r-1、cbrt(r³+1)=r
    for (u64 r : {2ull, 100ull, 2642245ull}) {
        u64 c = r * r * r;
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(c), r);
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(c - 1), r - 1);
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(c + 1), r);
    }
    // cbrt_3：3 limb 输入 floor 恒等式（(r+1)^3 经库原语重建）
    for (int iter = 0; iter < 150; ++iter) {
        mp_limb_t x[3];
        random_limbs(x, 3, seed, false);
        if (x[1] == 0) x[1] = 1;  // 契约至少 65 bit
        u64 c = lmmp_cbrt_3_(x[0], x[1], x[2]);
        mp_limb_t rc[1] = {c};
        TEST_CHECK_MSG(cbrt_floor_ok(rc, 1, x, 3), "cbrt_3 floor property");
    }
}

TEST_CASE("numth/cbrt", cbrt_6_divide_dispatcher) {
    u64 seed = 0x3cb7135c1f2a9e11ull;
    for (int shape = 0; shape < 3; ++shape) {
        for (mp_size_t ns : {2, 4}) {  // 输入 3*ns limbs
            for (int iter = 0; iter < 4; ++iter) {
                mp_size_t na = 3 * ns;
                mp_ptr numa = alloc_limbs(na), numa2 = alloc_limbs(na),
                       dst = alloc_limbs(ns + 1), t1 = alloc_limbs(2 * ns + 2),
                       t2 = alloc_limbs(3 * ns + 2), t3 = alloc_limbs(3 * ns + 8);
                random_limbs(numa, na, seed);
                if (shape == 1) {  // 完全立方：t^3（库 pow 链）；t 顶三位置 1
                    mp_ptr tt = alloc_limbs(ns);   // 保证 t^3 顶 limb >= 0x6000...
                    random_limbs(tt, ns, seed);
                    tt[ns - 1] |= (u64)0xE << 60;
                    lmmp_sqr_(t1, tt, ns);
                    lmmp_mul_(numa, t1, 2 * ns, tt, ns);
                    lmmp_free(tt);
                } else if (shape == 2) {
                    mp_ptr tt = alloc_limbs(ns);
                    random_limbs(tt, ns, seed);
                    tt[ns - 1] |= (u64)0xE << 60;
                    lmmp_sqr_(t1, tt, ns);
                    lmmp_mul_(numa, t1, 2 * ns, tt, ns);
                    sub_small_inplace(numa, na, 1);
                    lmmp_free(tt);
                }
                // random 形态顶 limb 已含 bit63（>= 0x8000 > 0x6000 契约）；构造形态
                // 由 tt 顶三位保证，不得再改 numa（保持完全立方语义）
                // cbrt_6_（3<na<=6）或 cbrt_divide_（na=3ns）
                if (na <= 6) {
                    lmmp_cbrt_6_(dst, numa, na);
                } else {
                    mp_ptr tp = alloc_limbs(6 * ns + 4);
                    memcpy(numa2, numa, (size_t)na * 8);
                    lmmp_cbrt_divide_(dst, numa2, ns, tp, 1);
                    lmmp_free(tp);
                }
                mp_size_t rn = norm_len(dst, ns);
                TEST_CHECK_MSG(cbrt_floor_ok(dst, rn > 0 ? rn : 1, numa, na),
                               "cbrt_6/divide floor");
                if (shape == 1 && na > 6)
                    TEST_CHECK_MSG(norm_len(numa2, 2 * ns) == 0,
                                   "perfect cube rem 0 (divide)");
                lmmp_free(numa); lmmp_free(numa2); lmmp_free(dst);
                lmmp_free(t1); lmmp_free(t2); lmmp_free(t3);
            }
        }
    }
    // cbrt_ 生产入口：nf=0 精确 floor；完全立方 ±1 构造；与 cbrt_ulong 互比
    for (mp_size_t na : {1, 2, 3, 5, 8, 16}) {
        for (int family = 0; family < 3; ++family) {
            mp_ptr numa = alloc_limbs(na);
            random_limbs(numa, na, seed);
            if (numa[na - 1] == 0) numa[na - 1] = 1;
            if (family > 0 && na % 3 == 0) {
                mp_size_t nt = na / 3;
                mp_ptr tt = alloc_limbs(nt), sq = alloc_limbs(2 * nt);
                random_limbs(tt, nt, seed);
                lmmp_sqr_(sq, tt, nt);
                lmmp_mul_(numa, sq, 2 * nt, tt, nt);
                if (family == 2) sub_small_inplace(numa, na, 1);
                lmmp_free(tt); lmmp_free(sq);
            }
            mp_size_t ns = na / 3 + 1;
            mp_ptr dsts = alloc_limbs(ns + 4), dstr = alloc_limbs(2 * ns + 4),
                   t1 = alloc_limbs(2 * ns + 4), t2 = alloc_limbs(3 * ns + 4),
                   t3 = alloc_limbs(3 * ns + 10);
            lmmp_zero(dsts, ns + 4);   // 根/余数短于缓冲时高位不得混入垃圾
            lmmp_zero(dstr, 2 * ns + 4);
            lmmp_cbrt_(dsts, dstr, numa, na, 0);
            mp_size_t rn = norm_len(dsts, ns);
            TEST_CHECK_MSG(cbrt_floor_ok(dsts, rn > 0 ? rn : 1, numa, na),
                           "cbrt_ floor identity");
            // 余数恒等式：n == r³ + rem
            {
                lmmp_sqr_(t1, dsts, rn > 0 ? rn : 1);
                lmmp_mul_(t2, t1, 2 * rn, dsts, rn);
                mp_ptr sum = alloc_limbs(3 * ns + 6);
                pad_add(t2, 3 * rn, dstr, 2 * ns + 1, 3 * ns + 6, sum);
                TEST_CHECK_MSG(mpn_eq(sum, 3 * ns + 6, numa, na), "cbrt_ n == r^3 + rem");
                lmmp_free(sum);
            }
            // 单 limb 输入与 cbrt_ulong_ 互比
            if (na == 1)
                TEST_CHECK_EQ(dsts[0], lmmp_cbrt_ulong_(numa[0]));
            lmmp_free(numa); lmmp_free(dsts); lmmp_free(dstr);
            lmmp_free(t1); lmmp_free(t2); lmmp_free(t3);
        }
    }
}

TEST_CASE("numth/cbrt", invcbrt_newton_semantics) {
    // I-1 <= ir <= I（绝不高估）；I = floor(cbrt(B^(3ns+na)/a)) 经库链构造
    u64 seed = 0x11c0ffee22c0ffeeull;
    for (mp_size_t na : {1, 3, 8}) {
        for (mp_size_t ns : {4, 8}) {
            if (ns == na) continue;
            mp_ptr a = alloc_limbs(na), dstis = alloc_limbs(ns + 2), icq = alloc_limbs(ns + 2),
                   nb = alloc_limbs(3 * ns + na + 4), q = alloc_limbs(3 * ns + 4);
            for (int f = 0; f < 3; ++f) {
                lmmp_zero(a, na);
                if (f < 2) {
                    random_limbs(a, na, seed);
                    a[na - 1] |= (u64)1 << 61;
                } else {
                    for (mp_size_t i = 0; i < na; ++i) a[i] = ~(u64)0;
                }
                lmmp_zero(dstis, ns + 2);
                lmmp_invcbrt_newton_(dstis, ns, a, na);
                TEST_CHECK_MSG(dstis[ns] >= 1, "invcbrt top limb >= 1");
                // I = floor(cbrt(B^(3ns+na)/a))
                mp_size_t e = 3 * ns + na;
                mp_size_t L = e + 1;
                lmmp_zero(nb, L);
                nb[L - 1] = 1;
                lmmp_div_(q, NULL, nb, L, a, na);
                mp_size_t ql = L - na + 1;
                while (ql > 1 && q[ql - 1] == 0) --ql;
                lmmp_zero(icq, ns + 2);  // 根短时高位不得混入垃圾
                lmmp_cbrt_(icq, NULL, q, ql, 0);
                mp_size_t il = (ql + 2) / 3 + 1;
                mp_ptr im1 = alloc_limbs(ns + 2);
                lmmp_zero(im1, ns + 2);
                memcpy(im1, icq, (size_t)il * 8);
                sub_small_inplace(im1, ns + 2, 1);
                bool ok = memcmp(dstis, icq, (size_t)(ns + 1) * 8) == 0 ||
                          memcmp(dstis, im1, (size_t)(ns + 1) * 8) == 0;
                TEST_CHECK_MSG(ok, "invcbrt in {I-1, I}");
                lmmp_free(im1);
            }
            lmmp_free(a); lmmp_free(dstis); lmmp_free(icq); lmmp_free(nb); lmmp_free(q);
        }
    }
}

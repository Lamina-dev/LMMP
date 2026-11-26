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

/* cbrt 家族验证（无 BigInt oracle；sqrt 见 test_numth_sqrt.cpp）：
   1. floor 恒等式：r³ ≤ n < (r+1)³（(r+1)^3 = r³+3r²+3r+1 经库乘加重建）、
      余数恒等式 n == r³ + rem
   2. 标量档（u64）：icbrt 二分参考（64 位内）+ 完全立方角点
   3. 入口互比：cbrt_3/cbrt_6/cbrt_divide_/cbrt_ 生产入口；完全立方构造
      （sqr+mul 后 ±1）命中精确语义
   4. 逆立方根语义：invcbrt 下近似区间 [I-1, I]（参照经 div_+cbrt_ 库链构造）
   5. 牛顿分派门：lmmp_cbrt_（dstr=NULL）恰在 na/A0 + ln(C) <= ln(ns) 时走牛顿，
      A0 = CBRT_INVNEWTON_EXP_SCALE、C = CBRT_INVNEWTON_MIN_ROOT、ns = (na+2)/3+nf+1 为
      根长 limb 数；覆盖分界线两侧、整数部恰在根长给定时的上限两侧、根长很小、dstr != NULL
*/

#include <math.h>

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "cases.h"
#include "mparam.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

// u64 isqrt 简单参考（二分，64 位内；root=2 交叉用）
static u64 isqrt_u64(u64 x) {
    if (x == 0) return 0;
    u64 lo = 1, hi = 0xffffffffull;
    while (lo < hi) {
        u64 mid = lo + (hi - lo + 1) / 2;
        if (mid <= x / mid) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

static u64 icbrt_u64(u64 x) {
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
static bool cbrt_floor_ok(const mp_limb_t* r, mp_size_t rn, const mp_limb_t* n, mp_size_t na) {
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

TEST_CASE("numth/cbrt", cbrt_scalar_and_cbrt_3) {
    u64 seed = 0x0badcafef00dfaceull;
    for (int i = 0; i < 1000; ++i) {
        u64 x = xorshift64(&seed);
        if (x == 0) x = 1;
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(x), icbrt_u64(x));
        TEST_CHECK_EQ(lmmp_cbrt_chebyshev_(x), icbrt_u64(x));
        TEST_CHECK_EQ(lmmp_nthroot_ulong_(x, 3), icbrt_u64(x));
        TEST_CHECK_EQ(lmmp_nthroot_ulong_(x, 2), isqrt_u64(x));  // root=2 分派 sqrt_ulong_
    }
    TEST_CHECK_EQ(lmmp_cbrt_ulong_(1), 1u);
    TEST_CHECK_EQ(lmmp_cbrt_ulong_(UINT64_MAX), 2642245ull);  // floor(cbrt(2^64-1))
    // 完全立方角点（u64 内）：cbrt(r³)=r、cbrt(r³-1)=r-1、cbrt(r³+1)=r
    const u64 r_values[] = {2ull, 100ull, 2642245ull};
    for (size_t r_index = 0; r_index < ARRAY_SIZE(r_values); ++r_index) {
        u64 r = r_values[r_index];
        u64 c = r * r * r;
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(c), r);
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(c - 1), r - 1);
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(c + 1), r);
    }
    // cbrt_3：3 limb 输入 floor 恒等式（(r+1)^3 经库原语重建）
    for (int iter = 0; iter < 150; ++iter) {
        mp_limb_t x[3];
        random_limbs_with_top(x, 3, &seed, false);
        if (x[1] == 0) x[1] = 1;  // 契约至少 65 bit
        u64 c = lmmp_cbrt_3_(x[0], x[1], x[2]);
        mp_limb_t rc[1] = {c};
        TEST_CHECK_MSG(cbrt_floor_ok(rc, 1, x, 3), "cbrt_3 floor property");
    }
}

TEST_CASE("numth/cbrt", cbrt_6_divide_dispatcher) {
    u64 seed = 0x3cb7135c1f2a9e11ull;
    for (int shape = 0; shape < 3; ++shape) {
        const mp_size_t ns_values[] = {2, 4};
        for (size_t ns_index = 0; ns_index < ARRAY_SIZE(ns_values); ++ns_index) {
            mp_size_t ns = ns_values[ns_index];  // 输入 3*ns limbs
            for (int iter = 0; iter < 4; ++iter) {
                mp_size_t na = 3 * ns;
                mp_ptr numa = alloc_limbs(na), numa2 = alloc_limbs(na),
                       dst = alloc_limbs(ns + 1), t1 = alloc_limbs(2 * ns + 2),
                       t2 = alloc_limbs(3 * ns + 2), t3 = alloc_limbs(3 * ns + 8);
                random_limbs(numa, na, &seed);
                if (shape == 1) {  // 完全立方：t^3（库 pow 链）；t 顶三位置 1
                    mp_ptr tt = alloc_limbs(ns);   // 保证 t^3 顶 limb >= 0x6000...
                    random_limbs(tt, ns, &seed);
                    tt[ns - 1] |= (u64)0xE << 60;
                    lmmp_sqr_(t1, tt, ns);
                    lmmp_mul_(numa, t1, 2 * ns, tt, ns);
                    lmmp_free(tt);
                } else if (shape == 2) {
                    mp_ptr tt = alloc_limbs(ns);
                    random_limbs(tt, ns, &seed);
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
                    if (na == 6) {
                        // cbrt6_fast_ 直接入口：结果与 cbrt_6_ 一致 + 余数恒等式
                        // [numa2,5] = [numa,6] - [dst,2]^3（立方经库乘重建）
                        mp_ptr tp = alloc_limbs(8), dst2 = alloc_limbs(2);
                        memcpy(numa2, numa, (size_t)na * 8);
                        lmmp_cbrt_6_fast_(dst2, numa2, tp);
                        TEST_CHECK_MSG(dst2[0] == dst[0] && dst2[1] == dst[1],
                                       "cbrt6_fast matches cbrt_6");
                        mp_ptr s2 = alloc_limbs(4), c3 = alloc_limbs(6);
                        lmmp_sqr_(s2, dst2, 2);
                        lmmp_mul_(c3, s2, 4, dst2, 2);
                        mp_ptr rem = alloc_limbs(6);
                        for (mp_size_t i = 0; i < 6; ++i)
                            rem[i] = i < na ? numa[i] : 0;
                        TEST_CHECK_MSG(lmmp_sub_(rem, rem, 6, c3, 6) == 0 &&
                                           mpn_cmp(rem, 6, numa2, 5) == 0,
                                       "cbrt6_fast remainder identity");
                        lmmp_free(tp); lmmp_free(dst2); lmmp_free(s2);
                        lmmp_free(c3); lmmp_free(rem);
                    }
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
    const mp_size_t na_values[] = {1, 2, 3, 5, 8, 16};
    for (size_t na_index = 0; na_index < ARRAY_SIZE(na_values); ++na_index) {
        mp_size_t na = na_values[na_index];
        for (int family = 0; family < 3; ++family) {
            mp_ptr numa = alloc_limbs(na);
            random_limbs(numa, na, &seed);
            if (numa[na - 1] == 0) numa[na - 1] = 1;
            if (family > 0 && na % 3 == 0) {
                mp_size_t nt = na / 3;
                mp_ptr tt = alloc_limbs(nt), sq = alloc_limbs(2 * nt);
                random_limbs(tt, nt, &seed);
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
    const mp_size_t na_values[] = {1, 3, 8};
    for (size_t na_index = 0; na_index < ARRAY_SIZE(na_values); ++na_index) {
        mp_size_t na = na_values[na_index];
        const mp_size_t ns_values[] = {4, 8};
        for (size_t ns_index = 0; ns_index < ARRAY_SIZE(ns_values); ++ns_index) {
            mp_size_t ns = ns_values[ns_index];
            if (ns == na) continue;
            mp_ptr a = alloc_limbs(na), dstis = alloc_limbs(ns + 2), icq = alloc_limbs(ns + 2),
                   nb = alloc_limbs(3 * ns + na + 4), q = alloc_limbs(3 * ns + 4);
            for (int f = 0; f < 3; ++f) {
                lmmp_zero(a, na);
                if (f < 2) {
                    random_limbs(a, na, &seed);
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

/* 牛顿分派门的判定式，与 lmmp_cbrt_ 的分派式相同 */
static bool gate_takes_newton(mp_size_t na, mp_size_t ns) {
    return (double)na / CBRT_INVNEWTON_EXP_SCALE + log(CBRT_INVNEWTON_MIN_ROOT) <= log((double)ns);
}

/* 随机找一个让两条分支结果必然不同的被开方数：[numa,na] 取归一化随机值，用带余数的精确路径
   多算 1 个 limb，得 R = floor(cbrt([numa,na]*B^(3*(nf+1))))，其最低 limb 是根的小数部分的
   前 64 位。小数部分落在 [1/2 + 2^-10, 1 - 2^-10] 内时，牛顿的 round 根恰比分治的 floor 根
   F = [R+1,ns] 大 1，接受该输入；否则重新随机。F 写入已清零的 [f,ns+2]，尝试耗尽返回 false。

   入口的牛顿分支先左移 3t 位归一化、算完再右移 t 位，结果是 floor(x + 2^(-t-1))，仅当 t = 0
   才是 round(x)；na 为 3 的倍数且顶 limb 最高位为 1 时 t = 0，其余 na 下几乎恒得 floor 根 */
static bool build_discriminating(mp_ptr numa, mp_size_t na, mp_size_t ns, mp_ptr f, u64 *seed) {
    const mp_size_t nf = ns - 1 - (na + 2) / 3;
    const mp_limb_t low = 0x8000000000000000ull + (1ull << 54), high = 0xffffffffffffffffull - (1ull << 54);
    mp_ptr r = alloc_limbs(ns + 4), rem = alloc_limbs(2 * ns + 6);
    bool found = false;
    for (int tries = 0; tries < 64 && !found; ++tries) {
        random_limbs(numa, na, seed);
        lmmp_zero(r, ns + 4);
        lmmp_zero(rem, 2 * ns + 6);
        lmmp_cbrt_(r, rem, numa, na, nf + 1);
        found = r[0] >= low && r[0] <= high;
    }
    if (found)
        lmmp_copy(f, r + 1, ns);
    lmmp_free(r);
    lmmp_free(rem);
    return found;
}

/* 形状 (na, ns)，ns 为根长 limb 数：dstr=NULL 的结果须为 F（走分治）或 F+1（走牛顿）；na 为
   3 的倍数时两条分支的结果必然不同，实际走的分支须与判定式一致。分派门会通过的形状另调带余数
   入口，须无视分派门给出 F。floor_oracle 另验 F^3 <= numa*B^(3nf) < (F+1)^3；rem_oracle 的
   形状另调带余数入口并验余数恒等式 numa*B^(3nf) == F^3 + rem */
static bool check_gate_shape(mp_size_t na, mp_size_t ns, u64 *seed, bool floor_oracle, bool rem_oracle) {
    const mp_size_t nf = ns - 1 - (na + 2) / 3;
    const bool telling = na % 3 == 0;  // 此时分派走哪条分支可由结果读出
    mp_ptr numa = alloc_limbs(na), f = alloc_limbs(ns + 2), r = alloc_limbs(ns + 2), t = alloc_limbs(ns + 2),
           rem = alloc_limbs(2 * ns + 4);
    lmmp_zero(f, ns + 2);
    bool ok = build_discriminating(numa, na, ns, f, seed);

    lmmp_zero(r, ns + 2);
    lmmp_cbrt_(r, NULL, numa, na, nf);
    lmmp_sub_n_(t, r, f, ns);
    const mp_size_t tn = norm_len(t, ns);
    const bool via_newton = tn == 1 && t[0] == 1;
    ok = ok && tn <= 1 && (!telling || via_newton == gate_takes_newton(na, ns));

    // 带余数入口：分派门会通过的形状须无视分派门仍走分治，给出精确 floor 根；
    // rem_oracle 的形状另验余数恒等式 numa*B^(3nf) == F^3 + rem
    if (ok && (rem_oracle || gate_takes_newton(na, ns))) {
        lmmp_zero(r, ns + 2);
        lmmp_zero(rem, 2 * ns + 4);
        lmmp_cbrt_(r, rem, numa, na, nf);
        ok = mpn_eq(r, ns + 2, f, ns + 2);
        if (ok && rem_oracle) {
            const mp_size_t bl = 3 * ns + 2;  // bl 不小于被开方数的 limb 数 na+3*nf
            mp_ptr big = alloc_limbs(bl), sq = alloc_limbs(2 * ns + 2), cube = alloc_limbs(3 * ns + 2);
            lmmp_zero(big, bl);
            lmmp_copy(big + 3 * nf, numa, na);  // big = numa*B^(3nf)
            lmmp_sqr_(sq, f, ns);
            lmmp_mul_(cube, sq, 2 * ns, f, ns);      // F^3
            lmmp_sub_(big, big, bl, cube, 3 * ns);   // numa*B^(3nf) - F^3
            ok = mpn_eq(big, bl, rem, 2 * ns + 4);
            lmmp_free(big); lmmp_free(sq); lmmp_free(cube);
        }
    }

    // floor 恒等式 F^3 <= numa*B^(3nf) < (F+1)^3，独立核对精确路径给出的 F
    if (ok && floor_oracle) {
        const mp_size_t bl = 3 * ns + 2;  // bl 不小于被开方数的 limb 数 na+3*nf
        mp_ptr big = alloc_limbs(bl), sq = alloc_limbs(2 * ns + 2), cube = alloc_limbs(3 * ns + 2),
               fp1 = alloc_limbs(ns + 2), sq1 = alloc_limbs(2 * ns + 4), cube1 = alloc_limbs(3 * ns + 6);
        lmmp_zero(big, bl);
        lmmp_copy(big + 3 * nf, numa, na);  // big = numa*B^(3nf)
        lmmp_sqr_(sq, f, ns);
        lmmp_mul_(cube, sq, 2 * ns, f, ns);  // F^3
        lmmp_zero(fp1, ns + 2);
        lmmp_copy(fp1, f, ns);
        add_small_inplace(fp1, ns + 1, 1);  // F+1
        const mp_size_t fn1 = norm_len(fp1, ns + 1);
        lmmp_sqr_(sq1, fp1, fn1);
        lmmp_mul_(cube1, sq1, 2 * fn1, fp1, fn1);  // (F+1)^3
        ok = mpn_cmp(cube, 3 * ns, big, bl) <= 0 && mpn_cmp(big, bl, cube1, 3 * fn1) < 0;
        lmmp_free(big); lmmp_free(sq); lmmp_free(cube); lmmp_free(fp1); lmmp_free(sq1); lmmp_free(cube1);
    }
    lmmp_free(numa); lmmp_free(f); lmmp_free(r); lmmp_free(t); lmmp_free(rem);
    return ok;
}

TEST_CASE("numth/cbrt", cbrt_newton_dispatch_gate) {
    u64 seed = 0xc0ffeebabe5eed1bull;
    // (1) 固定 na（3 的倍数，两分支的结果可区分），根长取分界线 C*exp(na/A0) 两侧最近的整数：
    //     分界线含超越数，整数形状不会恰在线上，线下的 line-1 一律分治，线上的 line+1 一律牛顿。
    //     每次核对要调 4~5 次大尺寸开方，大 na 的列只留两侧各一个形状；根长超过 25000 的形状
    //     不算 floor 恒等式的全立方
    const mp_size_t na_values[] = {3, 6, 15, 63, 138, 255, 381};
    for (size_t na_index = 0; na_index < ARRAY_SIZE(na_values); ++na_index) {
        const mp_size_t na = na_values[na_index];
        const mp_size_t line = (mp_size_t)(CBRT_INVNEWTON_MIN_ROOT * exp((double)na / CBRT_INVNEWTON_EXP_SCALE));
        const bool big = line > 25000;
        TEST_CHECK_MSG(check_gate_shape(na, line - 1, &seed, !big, false), "cbrt_ gate: one limb below the line");
        TEST_CHECK_MSG(check_gate_shape(na, line + 1, &seed, false, false), "cbrt_ gate: one limb above the line");
        if (na <= 63)
            TEST_CHECK_MSG(check_gate_shape(na, line, &seed, true, false), "cbrt_ gate: at floor of the line");
    }
    // (2) 固定根长，整数部取上限 a_max = A0*(ln L - ln C) 两侧最近的 3 的倍数：不超过上限走牛顿，
    //     越过上限走分治
    const mp_size_t ns_values[] = {4200, 12000};
    for (size_t ns_index = 0; ns_index < ARRAY_SIZE(ns_values); ++ns_index) {
        const mp_size_t ns = ns_values[ns_index];
        const double limit =
            CBRT_INVNEWTON_EXP_SCALE * (log((double)ns) - log(CBRT_INVNEWTON_MIN_ROOT));
        const mp_size_t within = 3 * (mp_size_t)(limit / 3.0);
        TEST_CHECK_MSG(within < limit && limit < within + 3, "na limit lies between two multiples of 3");
        TEST_CHECK_MSG(check_gate_shape(within, ns, &seed, true, true), "cbrt_ gate: na within the limit");
        TEST_CHECK_MSG(check_gate_shape(within + 3, ns, &seed, true, true), "cbrt_ gate: na past the limit");
    }
    // (3) 根长很小：分派门恒不过，恒为分治的 floor 根
    const mp_size_t small_na[] = {3, 6, 9};
    const mp_size_t small_nf[] = {2, 5, 30};
    for (size_t na_index = 0; na_index < ARRAY_SIZE(small_na); ++na_index)
        for (size_t nf_index = 0; nf_index < ARRAY_SIZE(small_nf); ++nf_index) {
            const mp_size_t ns = (small_na[na_index] + 2) / 3 + small_nf[nf_index] + 1;
            TEST_CHECK_MSG(ns < CBRT_INVNEWTON_MIN_ROOT, "small root length stays below C");
            TEST_CHECK_MSG(check_gate_shape(small_na[na_index], ns, &seed, true, true),
                           "cbrt_ gate: tiny root length never takes newton");
        }
    // (4) na 不是 3 的倍数：归一化带移位，牛顿分支经还原后几乎恒得 floor 根，不能据结果区分分支，
    //     只验契约（结果属于 {F, F+1}）；分界线两侧各取一个形状，大 na 只取牛顿一侧
    const mp_size_t shifted_na[] = {1, 4, 16, 64, 139, 256};
    for (size_t na_index = 0; na_index < ARRAY_SIZE(shifted_na); ++na_index) {
        const mp_size_t na = shifted_na[na_index];
        const mp_size_t line = (mp_size_t)(CBRT_INVNEWTON_MIN_ROOT * exp((double)na / CBRT_INVNEWTON_EXP_SCALE));
        TEST_CHECK_MSG(check_gate_shape(na, line + 1, &seed, false, false), "cbrt_ shifted gate: above the line");
        if (na <= 64)
            TEST_CHECK_MSG(check_gate_shape(na, line - 1, &seed, true, false), "cbrt_ shifted gate: below the line");
    }
}

void test_cbrt(void) {
    TEST_RUN("numth/cbrt", cbrt_scalar_and_cbrt_3);
    TEST_RUN("numth/cbrt", cbrt_6_divide_dispatcher);
    TEST_RUN("numth/cbrt", invcbrt_newton_semantics);
    TEST_RUN("numth/cbrt", cbrt_newton_dispatch_gate);
}

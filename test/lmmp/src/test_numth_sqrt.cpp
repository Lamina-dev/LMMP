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

/* sqrt 家族验证（无 BigInt oracle；cbrt 见 test_numth_cbrt.cpp）：
   1. floor 恒等式：r² ≤ n < (r+1)²（(r+1)^2 经库 sqr 重建），余数恒等式
      n == r^2 + rem（库加重建）
   2. 标量档（u64）：isqrt 二分参考（64 位内）
   3. 入口互比：sqrt_1/2/3/4、sqrt_divide_（强制分治）、sqrt_newton_、
      sqrt_ 生产入口彼此一致；完全平方构造（sqr 后 ±1）命中精确语义
   4. 逆根语义：invsqrt 下近似区间 [I-1, I]（参照经 div_+sqrt_ 库链构造）
   5. perfsqr：完全平方必过过滤器、sqrtrem 余数为零判据一致
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

// u64 isqrt 简单参考（二分，64 位内）
u64 isqrt_u64(u64 x) {
    if (x == 0) return 0;
    u64 lo = 1, hi = 0xffffffffull;
    while (lo < hi) {
        u64 mid = lo + (hi - lo + 1) / 2;
        // mid*mid <= x ？mid <= 2^32-1 ⟹ mid*mid 不溢出
        if (mid <= x / mid) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

// r² ≤ n < (r+1)²（r 为 [rs,rn]）。sqr_ 契约 sep(dst,numa)：独立缓冲；
// 长度取写入长度（mul/sqr 恒写满 na+nb/2na），比较走归一化 mpn_cmp。
bool sqrt_floor_ok(const mp_limb_t* r, mp_size_t rn, const mp_limb_t* n, mp_size_t na) {
    mp_ptr r2 = alloc_limbs(2 * rn);
    mp_ptr rp1 = alloc_limbs(rn + 2);
    mp_ptr p1 = alloc_limbs(2 * rn + 4);
    lmmp_sqr_(r2, r, rn);                       // r^2
    lmmp_zero(rp1, rn + 2);
    memcpy(rp1, r, (size_t)rn * 8);
    add_small_inplace(rp1, rn + 2, 1);          // r+1
    mp_size_t r1n = norm_len(rp1, rn + 2);
    if (r1n == 0) r1n = 1;
    lmmp_sqr_(p1, rp1, r1n);                    // (r+1)^2（独立缓冲）
    bool ok = mpn_cmp(r2, 2 * rn, n, na) <= 0 && mpn_cmp(n, na, p1, 2 * r1n) < 0;
    lmmp_free(r2);
    lmmp_free(rp1);
    lmmp_free(p1);
    return ok;
}

// n == r^2 + rem（sqrtrem 余数恒等式）
bool rem_sum_ok(const mp_limb_t* r, mp_size_t rn, const mp_limb_t* rem, mp_size_t remlen,
                const mp_limb_t* n, mp_size_t na, mp_size_t mx) {
    mp_ptr sq = alloc_limbs(2 * rn + 2), sum = alloc_limbs(mx);
    lmmp_sqr_(sq, r, rn);
    pad_add(sq, 2 * rn, rem, remlen, mx, sum);
    bool ok = mpn_eq(sum, mx, n, na);
    lmmp_free(sq);
    lmmp_free(sum);
    return ok;
}

}  // namespace

TEST_CASE("numth/sqrt", sqrt_ulong_sqrt_1_sqrt_2) {
    u64 seed = 0x0bad1dea0bad2deaull;
    for (int i = 0; i < 2000; ++i) {
        u64 x = xorshift64(seed);
        TEST_CHECK_EQ(lmmp_sqrt_ulong_(x), isqrt_u64(x));
        // sqrt_1 契约域 x >= B/4：强制顶两位置后随机
        u64 x1 = x | ((u64)3 << 62);
        mp_ptr r1 = alloc_limbs(1);
        mp_limb_t s = lmmp_sqrt_1_(r1, x1);
        TEST_CHECK_MSG(s == isqrt_u64(x1) && r1[0] == x1 - s * s, "sqrt_1 floor+rem");
        lmmp_free(r1);
    }
    // 写死角点
    TEST_CHECK_EQ(lmmp_sqrt_ulong_(0), 0u);
    TEST_CHECK_EQ(lmmp_sqrt_ulong_(1), 1u);
    TEST_CHECK_EQ(lmmp_sqrt_ulong_(3), 1u);
    TEST_CHECK_EQ(lmmp_sqrt_ulong_(4), 2u);
    TEST_CHECK_EQ(lmmp_sqrt_ulong_(UINT64_MAX), 0xffffffffull);
    {
        mp_ptr r1 = alloc_limbs(1);
        u64 m = UINT64_MAX;
        TEST_CHECK_MSG(lmmp_sqrt_1_(r1, m) == 0xffffffffull, "sqrt_1 max");
        // (2^32-1)^2 = 2^64 - 2^33 + 1 → rem = m - s²
        TEST_CHECK_MSG(r1[0] == m - 0xffffffffull * 0xffffffffull, "sqrt_1 max rem");
        lmmp_free(r1);
    }
    // sqrt_2：随机双 limb（顶 limb >= B/4）
    for (int i = 0; i < 500; ++i) {
        mp_ptr x = alloc_limbs(2), r2 = alloc_limbs(2);
        random_limbs(x, 2, seed);
        x[1] |= (u64)1 << 62;
        mp_limb_t s = lmmp_sqrt_2_(r2, x);
        // 契约 x >= B^2/4 ⟹ s >= 2^63；x < B^2 ⟹ s < B
        TEST_CHECK_MSG(s >= (1ull << 63), "sqrt_2 range");
        // (s+1)^2 > x >= s^2（mullh 全积比较；s 顶值时 (s+1)^2 = B^2 > x 恒真）
        mp_limb_t p[2], q[2];
        lmmp_mullh_(s, s, p);
        bool ok = lmmp_cmp_(p, x, 2) <= 0;
        if (ok) {
            if (s == UINT64_MAX) ok = true;
            else {
                lmmp_mullh_(s + 1, s + 1, q);
                ok = lmmp_cmp_(x, q, 2) < 0;
            }
        }
        TEST_CHECK_MSG(ok, "sqrt_2 floor identity");
        // 余数 = x - s^2（借位减后与 sqrt_2 写出的余数一致）
        lmmp_sub_n_(p, x, p, 2);
        TEST_CHECK_MSG(memcmp(p, r2, 16) == 0, "sqrt_2 rem value");
        lmmp_free(x);
        lmmp_free(r2);
    }
}

TEST_CASE("numth/sqrt", sqrt_3_sqrt_4_and_divide) {
    u64 seed = 0x2d7a3f1b9e5c8247ull;
    for (int shape = 0; shape < 3; ++shape) {
        for (mp_size_t ns : {2, 4, 8}) {  // 输入 2*ns limbs
            for (int iter = 0; iter < 4; ++iter) {
                mp_size_t na = 2 * ns;
                mp_ptr numa = alloc_limbs(na), numa2 = alloc_limbs(na),
                       dst = alloc_limbs(ns),
                       t1 = alloc_limbs(2 * ns + 2), t2 = alloc_limbs(2 * ns + 2);
                random_limbs(numa, na, seed);
                if (shape == 1) {  // 完全平方：numa = t^2（t 顶两位置 1 保证
                    // t^2 顶 >= B/4 契约，构造后不得再改 numa）
                    mp_ptr tt = alloc_limbs(ns);
                    random_limbs(tt, ns, seed);
                    tt[ns - 1] |= (u64)3 << 62;
                    lmmp_sqr_(numa, tt, ns);
                    lmmp_free(tt);
                } else if (shape == 2) {  // t^2 - 1
                    mp_ptr tt = alloc_limbs(ns);
                    random_limbs(tt, ns, seed);
                    tt[ns - 1] |= (u64)3 << 62;
                    lmmp_sqr_(numa, tt, ns);
                    sub_small_inplace(numa, na, 1);
                    lmmp_free(tt);
                }
                memcpy(numa2, numa, (size_t)na * 8);

                lmmp_sqrt_divide_(dst, numa2, ns, t1, 1);  // 余数在 [numa2, ns+1]
                TEST_CHECK_MSG(sqrt_floor_ok(dst, ns, numa, na), "sqrt_divide floor");
                if (shape == 1)
                    TEST_CHECK_MSG(norm_len(numa2, ns) == 0, "perfect square rem 0");
                else
                    TEST_CHECK_MSG(norm_len(numa2, ns) > 0, "non-square rem > 0");
                // 余数恒等式 n == s² + rem
                TEST_CHECK_MSG(
                    rem_sum_ok(dst, ns, numa2, ns + 1, numa, na, 2 * ns + 4),
                    "sqrt_divide n == s^2 + rem");
                lmmp_free(numa); lmmp_free(numa2); lmmp_free(dst);
                lmmp_free(t1); lmmp_free(t2);
            }
        }
    }

    // sqrt_3_ / sqrt_4_ 特化直调：3/4 limb 输入，floor 恒等式 + 余数
    u64 seed34 = 0x34c0ffee34c0ffeeull;
    for (mp_size_t na : {3, 4}) {
        for (int iter = 0; iter < 8; ++iter) {
            mp_ptr numa = alloc_limbs(na), dsts = alloc_limbs(2), dstr = alloc_limbs(4),
                   t1 = alloc_limbs(8), t2 = alloc_limbs(8);
            random_limbs(numa, na, seed34);
            numa[na - 1] |= (u64)1 << 62;
            if (na == 3) lmmp_sqrt_3_(dsts, dstr, numa);
            else lmmp_sqrt_4_(dsts, dstr, numa);
            TEST_CHECK_MSG(sqrt_floor_ok(dsts, 2, numa, na), "sqrt_3/4 floor");
            TEST_CHECK_MSG(rem_sum_ok(dsts, 2, dstr, 3, numa, na, 12), "sqrt_3/4 rem identity");
            lmmp_free(numa); lmmp_free(dsts); lmmp_free(dstr); lmmp_free(t1); lmmp_free(t2);
        }
    }
}

TEST_CASE("numth/sqrt", sqrt_dispatcher_fractional) {
    // 生产入口 lmmp_sqrt_（nf=0 精确 floor；nf>0 [floor|round]）与强制分治互比；
    // 完全平方族（库 sqr 构造）在任意 nf 下必须精确命中 t（floor==round）
    u64 seed = 0x77aa55cc33ee1199ull;
    for (mp_size_t na : {1, 2, 3, 5, 8, 16, 33}) {
        for (int family = 0; family < 3; ++family) {
            bool constructed_square = false;  // 偶数长度才可构造 t^2 恰 na limb
            mp_ptr numa = alloc_limbs(na);
            mp_ptr tt = alloc_limbs(na / 2 + 1);
            if (family == 0 || (na & 1) == 1) {
                random_limbs(numa, na, seed);
            } else {
                // t^2 / t^2-1（库 sqr 构造；偶数长度才恰为 na limb）
                random_limbs(tt, na / 2, seed);
                lmmp_sqr_(numa, tt, na / 2);
                if (family == 1) constructed_square = true;
                if (family == 2) sub_small_inplace(numa, na, 1);
            }
            if (norm_len(numa, na) == 0) numa[0] = 1;
            if (numa[na - 1] == 0) numa[na - 1] = 1;  // 契约 numa[na-1]!=0

            for (mp_size_t nf : {0, 2, 7}) {
                mp_size_t ns = nf + na / 2 + 1;
                mp_ptr ref = alloc_limbs(ns + 2), rem = alloc_limbs(ns + 2), dst = alloc_limbs(ns + 2);
                lmmp_sqrt_(ref, rem, numa, na, nf);  // 精确 sqrtrem 参考
                {  // 余数界 rem < 2*root+1（(s+1)^2 = s^2+2s+1）
                    mp_ptr two_s = alloc_limbs(ns + 2);
                    lmmp_zero(two_s, ns + 2);
                    memcpy(two_s, ref, (size_t)ns * 8);
                    lmmp_add_n_(two_s, two_s, two_s, ns + 2);  // 2s（零垫域内翻倍）
                    add_small_inplace(two_s, ns + 2, 1);
                    TEST_CHECK_MSG(mpn_cmp(rem, ns, two_s, ns + 2) < 0, "sqrt_ rem < 2s+1");
                    lmmp_free(two_s);
                }
                // floor 恒等式：ref² ≤ numa*B^(2nf) < (ref+1)²
                {
                    mp_ptr sq = alloc_limbs(2 * ns + 2), rp1 = alloc_limbs(ns + 2),
                            big = alloc_limbs(na + 2 * nf + 2), t1 = alloc_limbs(2 * ns + 4),
                            t2 = alloc_limbs(2 * ns + 4);
                    mp_size_t rn = norm_len(ref, ns);
                    lmmp_sqr_(sq, ref, rn);
                    lmmp_zero(rp1, ns + 2);
                    memcpy(rp1, ref, (size_t)rn * 8);
                    add_small_inplace(rp1, ns + 2, 1);
                    mp_size_t r1n = norm_len(rp1, ns + 2);
                    lmmp_sqr_(t2, rp1, r1n);
                    // big = numa * B^(2nf)
                    lmmp_zero(big, na + 2 * nf + 2);
                    memcpy(big + 2 * nf, numa, (size_t)na * 8);
                    TEST_CHECK_MSG(mpn_cmp(sq, 2 * rn, big, na + 2 * nf) <= 0 &&
                                       mpn_cmp(big, na + 2 * nf, t2, 2 * r1n) < 0,
                                   "sqrt_ floor identity");
                    // 余数恒等式：big == ref² + rem
                    mp_ptr sum = alloc_limbs(2 * ns + 4);
                    pad_add(sq, 2 * rn, rem, ns, 2 * ns + 4, sum);
                    TEST_CHECK_MSG(mpn_eq(sum, 2 * ns + 4, big, na + 2 * nf),
                                   "sqrt_ n == s^2 + rem");
                    lmmp_free(sq); lmmp_free(rp1); lmmp_free(big);
                    lmmp_free(t1); lmmp_free(t2); lmmp_free(sum);
                }
                // dstr=NULL 路径与带余数同值
                lmmp_sqrt_(dst, NULL, numa, na, nf);
                TEST_CHECK_MSG(memcmp(dst, ref, (size_t)ns * 8) == 0, "sqrt_ null rem");
                // nf>=2：sqrt_newton_ ∈ {floor, floor+1}；完全平方必须精确
                if (nf >= 2) {
                    mp_ptr nw = alloc_limbs(ns + 2);
                    lmmp_sqrt_newton_(nw, numa, na, nf);
                    bool ok = memcmp(nw, ref, (size_t)ns * 8) == 0;
                    if (!ok) {  // 允许恰 +1（round 语义；完全平方除外）
                        mp_ptr rp1 = alloc_limbs(ns + 2);
                        lmmp_zero(rp1, ns + 2);
                        memcpy(rp1, ref, (size_t)ns * 8);
                        add_small_inplace(rp1, ns + 2, 1);
                        ok = memcmp(nw, rp1, (size_t)ns * 8) == 0;
                        lmmp_free(rp1);
                    }
                    bool must_exact = constructed_square;
                    if (must_exact) TEST_CHECK_MSG(memcmp(nw, ref, (size_t)ns * 8) == 0,
                                                   "newton perfect square exact");
                    else TEST_CHECK_MSG(ok, "newton in {floor, floor+1}");
                    lmmp_free(nw);
                }
                lmmp_free(ref); lmmp_free(rem); lmmp_free(dst);
            }
            lmmp_free(numa);
            lmmp_free(tt);
        }
    }
}

TEST_CASE("numth/sqrt", invsqrt_newton_semantics) {
    // I-1 <= ir <= I（绝不高估）；I 经 div_+sqrt_ 库链构造
    u64 seed = 0x5eed1dea5eed1deaull;
    for (mp_size_t na : {1, 3, 8}) {
        for (mp_size_t ns : {4, 8}) {
            if (ns == na) continue;
            mp_ptr a = alloc_limbs(na), dstis = alloc_limbs(ns + 2), isq = alloc_limbs(ns + 2),
                   nb = alloc_limbs(2 * ns + na + 4), q = alloc_limbs(2 * ns + 4);
            for (int f = 0; f < 4; ++f) {
                lmmp_zero(a, na);
                if (f < 2) {
                    random_limbs(a, na, seed);
                    a[na - 1] |= (u64)1 << 61;
                } else if (f == 2) {
                    a[na - 1] = 0x4000000000000000ull;  // a = B^na/4 边界
                } else {
                    for (mp_size_t i = 0; i < na; ++i) a[i] = ~(u64)0;  // B^na-1
                }
                lmmp_zero(dstis, ns + 2);
                lmmp_invsqrt_newton_(dstis, ns, a, na);
                TEST_CHECK_MSG(dstis[ns] == 1, "invsqrt top limb == 1");
                // I 参照：ns>=na 时 I = floor(sqrt(B^(2ns+na)/a))；
                // ns<na 时语义不同（仅用 a 的最高 ns limb，结果量纲差
                // B^((ns-na)/2)）：改为 floor(sqrt(B^(3ns)/a_top))
                mp_size_t e, adiv;
                const mp_limb_t* ap;
                if (ns >= na) {
                    e = 2 * ns + na;
                    ap = a;
                    adiv = na;
                } else {
                    e = 3 * ns;
                    ap = a + (na - ns);
                    adiv = ns;
                }
                mp_size_t L = e + 1;
                lmmp_zero(nb, L);
                nb[L - 1] = 1;
                lmmp_div_(q, NULL, nb, L, ap, adiv);
                mp_size_t ql = L - adiv + 1;
                while (ql > 1 && q[ql - 1] == 0) --ql;
                lmmp_zero(isq, ns + 2);  // 根短于 ns+1 时高位不得混入垃圾
                lmmp_sqrt_(isq, NULL, q, ql, 0);
                // ir ∈ {I-1, I}（等长 ns+1 比较）
                mp_ptr im1 = alloc_limbs(ns + 2);
                lmmp_zero(im1, ns + 2);
                memcpy(im1, isq, (size_t)(ns + 1) * 8);
                sub_small_inplace(im1, ns + 2, 1);
                bool ok = memcmp(dstis, isq, (size_t)(ns + 1) * 8) == 0 ||
                          memcmp(dstis, im1, (size_t)(ns + 1) * 8) == 0;
                TEST_CHECK_MSG(ok, "invsqrt in {I-1, I}");
                lmmp_free(im1);
            }
            lmmp_free(a); lmmp_free(dstis); lmmp_free(isq); lmmp_free(nb); lmmp_free(q);
        }
    }
}

TEST_CASE("numth/perfsqr", perfsqr_filter_perfsqr) {
    u64 seed = 0x1deafbee1deafbeeull;
    // filter_1：单 limb 契约域。完全平方（u64 内 r²）必过
    for (int i = 0; i < 200; ++i) {
        u64 r = 1 + (xorshift64(seed) & 0xffffffffull);  // r <= 2^32-1 ⟹ r² < B
        TEST_CHECK_MSG(lmmp_perfsqr_filter_1_(r * r), "filter_1 square pass");
    }
    // 完全平方必过过滤器；perfsqr_ 判据与 sqrtrem 余数一致
    for (mp_size_t nt : {1, 2, 5, 17}) {
        for (int iter = 0; iter < 6; ++iter) {
            mp_ptr tt = alloc_limbs(nt), sq = alloc_limbs(2 * nt), dstr = alloc_limbs(nt + 2);
            random_limbs(tt, nt, seed);
            lmmp_sqr_(sq, tt, nt);
            TEST_CHECK_MSG(lmmp_perfsqr_filter_(sq, 2 * nt), "filter square pass");
            TEST_CHECK_MSG(lmmp_perfsqr_(sq, 2 * nt), "perfsqr square true");
            // 非平方（平方 ±1，顶 limb 非零时）：perfsqr_ 必假；过滤器允许过
            if (sq[0] >= 2) {
                sq[0] -= 1;
                if (norm_len(sq, 2 * nt) > 0) {
                    TEST_CHECK_MSG(!lmmp_perfsqr_(sq, 2 * nt), "perfsqr square-1 false");
                }
                sq[0] += 2;
                if (sq[0] != 0) {
                    TEST_CHECK_MSG(!lmmp_perfsqr_(sq, 2 * nt), "perfsqr square+1 false");
                }
                sq[0] -= 1;
            }
            lmmp_free(tt); lmmp_free(sq); lmmp_free(dstr);
        }
    }
    // 随机非平方：perfsqr_ 与 sqrtrem 余数判据一致
    for (mp_size_t n : {1, 3, 8, 20}) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr x = alloc_limbs(n), dstr = alloc_limbs(n + 2);
            random_limbs(x, n, seed);
            bool isp = lmmp_perfsqr_(x, n);
            lmmp_sqrt_(dstr, NULL, x, n, 0);
            mp_size_t sn = n / 2 + 1;
            mp_ptr sq = alloc_limbs(2 * sn + 2);
            mp_size_t rn = norm_len(dstr, sn);
            lmmp_sqr_(sq, dstr, rn);
            bool rem_zero = mpn_eq(sq, 2 * rn, x, n);
            TEST_CHECK_MSG(isp == rem_zero, "perfsqr matches sqrtrem criterion");
            lmmp_free(x); lmmp_free(dstr); lmmp_free(sq);
        }
    }
}

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
#include "lmmp/impl/mparam.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed, bool msb = true) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0 && msb) p[n - 1] |= (u64)1 << 63;
}

u64 isqrt_u64(u64 n) {
    u64 lo = 0, hi = 1ull << 32;  // sqrt(2^64-1) < 2^32
    while (lo < hi) {
        u64 mid = (lo + hi + 1) >> 1;
        u128 sq = (u128)mid * mid;
        if (sq <= n) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

u64 icbrt_u64(u64 n) {
    u64 lo = 0, hi = 1ull << 22;  // cbrt(2^64-1) < 2^22
    while (lo < hi) {
        u64 mid = (lo + hi + 1) >> 1;
        u128 cu = (u128)mid * mid * mid;
        if (cu <= n) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

// 参考牛顿法整数平方根（小/中等规模）。
BigInt ref_isqrt(const BigInt& n) {
    if (n.is_zero()) return BigInt(0);
    BigInt x(1);
    size_t bits = n.d.size() * 64;
    x = BigInt::shl_bits(BigInt(1), (bits + 1) / 2);
    for (int iter = 0; iter < 200; ++iter) {
        BigInt rem;
        BigInt q = BigInt::divmod_school(n, x, rem);
        BigInt next = BigInt::add_abs(x, q);
        next = BigInt::shr_bits(next, 1);
        if (next == x) break;
        if (BigInt::cmp(next, x) >= 0 && BigInt::sub_abs(next, x).d.size() == 1 && BigInt::sub_abs(next, x).d[0] <= 1) {
            x = next;
            break;
        }
        x = next;
    }
    // 修正
    while (BigInt::sqr_school(BigInt::add_small(x, 1)) <= n) x = BigInt::add_small(x, 1);
    while (BigInt::sqr_school(x) > n) x = BigInt::sub_small(x, 1);
    return x;
}

bool ref_perfect_square(const BigInt& n) {
    BigInt s = ref_isqrt(n);
    return BigInt::sqr_school(s) == n;
}

}  // namespace

TEST_CASE("numth/sqrt", sqrt_ulong_sqrt_1_sqrt_2) {
    u64 seed = 0x31415bc81297c793ull;
    for (int i = 0; i < 1000; ++i) {
        u64 x = xorshift64(seed);
        u64 s = isqrt_u64(x);
        TEST_CHECK_EQ(lmmp_sqrt_ulong_(x), s);
    }
    TEST_CHECK_EQ(lmmp_sqrt_ulong_(0), 0u);
    TEST_CHECK_EQ(lmmp_sqrt_ulong_(UINT64_MAX), 0xffffffffull);

    for (int i = 0; i < 1000; ++i) {
        u64 x = xorshift64(seed);
        x = (x < LIMB_B_4) ? (x | LIMB_B_4) : x;
        mp_limb_t rem = 0;
        mp_limb_t s = lmmp_sqrt_1_(&rem, x);
        u128 sq = (u128)s * s;
        TEST_CHECK_MSG(sq + rem == x, "sqrt_1 sqrtrem relation");
        TEST_CHECK_MSG(sq <= x && (u128)(s + 1) * (s + 1) > x, "sqrt_1 floor property");
    }

    for (int i = 0; i < 500; ++i) {
        mp_limb_t x[2], rem[2];
        random_limbs(x, 2, seed);
        BigInt bx(x, 2);
        BigInt sref = ref_isqrt(bx);
        mp_limb_t s = lmmp_sqrt_2_(rem, x);
        BigInt bs(s);
        BigInt brem(rem, 2);
        TEST_CHECK_MSG(bs == sref, "sqrt_2 root");
        TEST_CHECK_MSG(BigInt::add_abs(BigInt::sqr_school(bs), brem) == bx, "sqrt_2 sqrtrem relation");
    }
}

TEST_CASE("numth/sqrt", sqrt_divide_and_sqrt) {
    u64 seed = 0x2718281828459045ull;
    for (mp_size_t ns : {1, 2, 5, 10, 20, 40}) {
        mp_size_t na = 2 * ns;
        mp_ptr numa = alloc_limbs(na);
        mp_ptr orig = alloc_limbs(na);
        mp_ptr dst = alloc_limbs(ns + 1);
        mp_ptr tp = alloc_limbs(3 * ns / 2 + 2);
        random_limbs(numa, na, seed);
        BigInt bn(numa, na);
        std::memcpy(orig, numa, na * 8);

        lmmp_sqrt_divide_(dst, numa, ns, tp, 1);
        BigInt bs(dst, ns);
        BigInt rem(numa, ns + 1);
        BigInt s2 = BigInt::sqr_school(bs);
        TEST_CHECK_MSG(BigInt::add_abs(s2, rem) == bn, "sqrt_divide sqrtrem relation");
        TEST_CHECK_MSG(rem < BigInt::add_small(BigInt::shl_bits(bs, 1), 1), "sqrt_divide remainder bound");
        TEST_CHECK_MSG(s2 <= bn && BigInt::sqr_school(BigInt::add_small(bs, 1)) > bn, "sqrt_divide floor property");

        lmmp_free(numa); lmmp_free(orig); lmmp_free(dst); lmmp_free(tp);
    }

    for (mp_size_t na : {1, 2, 5, 10}) {
        mp_size_t alloc_len = na / 2 + 1 + 2;
        mp_ptr numa = alloc_limbs(na);
        mp_ptr orig = alloc_limbs(na);
        mp_ptr dst = alloc_limbs(alloc_len);
        mp_ptr rem = alloc_limbs(alloc_len);
        random_limbs(numa, na, seed);
        BigInt bn(numa, na);
        std::memcpy(orig, numa, na * 8);

        lmmp_sqrt_(dst, rem, numa, na, 0);
        mp_size_t slen = na / 2 + 1;
        BigInt bs(dst, slen);
        BigInt br(rem, slen);
        BigInt s2 = BigInt::sqr_school(bs);
        TEST_CHECK_MSG(BigInt::add_abs(s2, br) == bn, "sqrt sqrtrem relation");
        TEST_CHECK_MSG(s2 <= bn && BigInt::sqr_school(BigInt::add_small(bs, 1)) > bn, "sqrt floor property");

        lmmp_free(numa); lmmp_free(orig); lmmp_free(dst); lmmp_free(rem);
    }
}

/*
    覆盖 lmmp_sqrt_ 的 dstr=NULL 分数开方两条路径（选择机制随阈值变化，
    故同时直接调用内部函数强制两条路径）：
      divide 路径（calr=0）结果必须是精确 floor；
      newton 路径结果为 floor 或 floor+1（最近舍入）。
    以 dstr!=NULL 的 sqrtrem 结果为精确 floor 参考；完全平方数（及其 -1
    邻域）覆盖 divide 快速判定的窄带回退与 newton 的舍入边界。
    divide 路径的低层输入构造复刻 lmmp_sqrt_ 的 else 分支。
*/
static void sqrt_fractional_divide_forced(mp_ptr dsts, mp_srcptr numa, mp_size_t na, mp_size_t nf,
                                          mp_ptr numa2, mp_ptr tp) {
    int nsh = lmmp_leading_zeros_(numa[na - 1]) / 2;
    mp_size_t ns = (na + 2 * nf + 1) / 2;
    lmmp_zero(numa2, 2 * nf);
    if (nsh)
        lmmp_shl_(numa2 + 2 * ns - na, numa, na, nsh * 2);
    else
        lmmp_copy(numa2 + 2 * ns - na, numa, na);
    if ((na + 2 * nf) & 1) {
        numa2[2 * nf] = 0;
        nsh += 32;  // LIMB_BITS/2，奇 nl 时被开方数额外左移了半个 limb
    } else {
        dsts[ns] = 0;
    }
    lmmp_sqrt_divide_(dsts, numa2, ns, tp, 0);
    if (nsh)
        lmmp_shr_(dsts, dsts, ns, nsh);
}

TEST_CASE("numth/sqrt", sqrt_fractional_divide_newton) {
    u64 seed = 0x2d7a3f1b9e5c8247ull;
    const mp_size_t nas[] = {2, 4, 7, 16, 33, 64};
    for (mp_size_t na : nas) {
        mp_size_t nfs[] = {1, na, 10 * na, 20 * na - 1, 20 * na, 20 * na + 64, 25 * na};
        for (mp_size_t nf : nfs) {
            mp_size_t ns = na / 2 + 1 + nf;
            mp_ptr numa = alloc_limbs(na);
            mp_ptr numa2 = alloc_limbs(2 * (na + nf) + 8);
            mp_ptr tp = alloc_limbs(3 * ns / 2 + 2);
            mp_ptr ref = alloc_limbs(ns + 2);
            mp_ptr rem = alloc_limbs(ns + 2);
            mp_ptr dst0 = alloc_limbs(ns + 2);  // 生产入口（路径由阈值决定）
            mp_ptr dst1 = alloc_limbs(ns + 2);  // 强制 divide
            mp_ptr dst2 = alloc_limbs(ns + 2);  // 强制 newton

            BigInt expect;  // 完全平方时的期望值 t*B^nf，否则为 0 表示无期望
            random_limbs(numa, na, seed);
            if ((na & 1) == 0) {
                // 每 2 轮切换输入族：随机 / t^2 / t^2-1
                static int family = 0;
                if (family == 1 || family == 2) {
                    mp_size_t nt = na / 2;
                    mp_ptr t = alloc_limbs(nt);
                    random_limbs(t, nt, seed);
                    BigInt bs = BigInt::sqr_school(from_limbs(t, nt));
                    if (family == 2)
                        bs = BigInt::sub_small(bs, 1);
                    std::memset(numa, 0, na * 8);
                    std::memcpy(numa, bs.d.data(), bs.d.size() * 8);
                    if (family == 1) {
                        expect = BigInt::shl_bits(from_limbs(t, nt), 64 * (size_t)nf);
                    }
                }
                family = (family + 1) % 3;
            }

            lmmp_zero(ref, ns + 2);
            lmmp_zero(rem, ns + 2);
            lmmp_zero(dst0, ns + 2);
            lmmp_zero(dst1, ns + 2);
            lmmp_zero(dst2, ns + 2);

            lmmp_sqrt_(ref, rem, numa, na, nf);  // 精确 floor 参考
            lmmp_sqrt_(dst0, NULL, numa, na, nf);
            sqrt_fractional_divide_forced(dst1, numa, na, nf, numa2, tp);
            BigInt bref = from_limbs(ref, ns);
            BigInt bexp = expect.is_zero() ? bref : expect;

            TEST_CHECK_MSG(from_limbs(dst1, ns) == bexp, "forced divide == exact floor");
            if (!expect.is_zero())
                TEST_CHECK_MSG(bref == bexp, "sqrtrem of perfect square");
            if (nf >= 2) {
                lmmp_sqrt_newton_(dst2, numa, na, nf);
                BigInt bnew = from_limbs(dst2, ns);
                BigInt bexp1 = BigInt::add_small(bexp, 1);
                if (!expect.is_zero()) {
                    // 被开方数为整数平方：floor == round，newton 必须精确命中
                    TEST_CHECK_MSG(bnew == bexp, "newton of perfect square exact");
                } else {
                    TEST_CHECK_MSG(bnew == bexp || bnew == bexp1, "newton in {floor, floor+1}");
                }
                BigInt b0 = from_limbs(dst0, ns);
                if (!expect.is_zero())
                    TEST_CHECK_MSG(b0 == bexp, "production call of perfect square exact");
                else
                    TEST_CHECK_MSG(b0 == bexp || b0 == bexp1, "production call in {floor, floor+1}");
            } else {
                BigInt b0 = from_limbs(dst0, ns);
                TEST_CHECK_MSG(b0 == bexp, "production call exact (nf<2, divide only)");
            }

            lmmp_free(numa); lmmp_free(numa2); lmmp_free(tp);
            lmmp_free(ref); lmmp_free(rem);
            lmmp_free(dst0); lmmp_free(dst1); lmmp_free(dst2);
        }
    }
}

/*
    lmmp_invsqrt_newton_ 误差语义：
      ns>=na 时记 I=floor(sqrt(B^(2ns+na)/a))，须有 I-1 <= ir <= I 且 dstis[ns]=1；
      ns<na 时实际计算 floor(sqrt(B^(3ns)/a_top))-[0|1]，a_top 为 a 的最高 ns limb。
    参照由 lmmp_div_（精确商）+ lmmp_sqrt_（精确 floor）构造。
*/
static void invsqrt_ref(mp_ptr isq, mp_srcptr a, mp_size_t nadiv, mp_size_t e, mp_ptr nb, mp_ptr q) {
    mp_size_t L = e + 1;  // B^e 共 e+1 个 limb，最高 limb 为 1
    lmmp_zero(nb, L);
    nb[L - 1] = 1;
    lmmp_div_(q, NULL, nb, L, a, nadiv);
    mp_size_t ql = L - nadiv + 1;
    while (ql > 1 && q[ql - 1] == 0) --ql;  // lmmp_sqrt_ 要求最高 limb 非零
    lmmp_sqrt_(isq, NULL, q, ql, 0);
}

TEST_CASE("numth/sqrt", invsqrt_newton_semantics) {
    u64 seed = 0x77aa55cc33ee1199ull;
    const mp_size_t nas[] = {1, 3, 8, 16, 33};
    const mp_size_t nss[] = {4, 8, 16, 40};
    for (mp_size_t na : nas) {
        for (mp_size_t ns : nss) {
            if (ns == na) continue;
            mp_size_t nsz = ns + 2;
            mp_ptr a = alloc_limbs(na + 1);
            mp_ptr dstis = alloc_limbs(nsz + 2);
            mp_ptr isq = alloc_limbs(nsz + 2);
            mp_ptr nb = alloc_limbs(2 * ns + na + 4);
            mp_ptr q = alloc_limbs(2 * ns + 4);

            for (int f = 0; f < 6; ++f) {
                lmmp_zero(a, na);
                if (f < 3) {
                    random_limbs(a, na, seed);
                } else if (f == 3) {
                    a[na - 1] = LIMB_B_4;  // a = B^na/4，I=2*B^ns 边界
                } else if (f == 4) {
                    a[na - 1] = LIMB_B_4;
                    a[0] += 1;
                } else {
                    for (mp_size_t i = 0; i < na; ++i) a[i] = ~(u64)0;  // B^na-1，I≈B^ns 边界
                }

                lmmp_zero(dstis, nsz + 2);
                lmmp_invsqrt_newton_(dstis, ns, a, na);
                TEST_CHECK_MSG(dstis[ns] == 1, "invsqrt top limb == 1");

                if (ns > na) {
                    invsqrt_ref(isq, a, na, 2 * ns + na, nb, q);
                } else {
                    invsqrt_ref(isq, a + (na - ns), ns, 3 * ns, nb, q);
                }
                BigInt bref = from_limbs(isq, ns + 1);
                BigInt bir = from_limbs(dstis, ns + 1);
                BigInt brefm1 = BigInt::sub_small(bref, 1);
                // ir 绝不高估，至多低估 1（ns<na 时相对其自身语义）
                TEST_CHECK_MSG(bir == brefm1 || bir == bref, "invsqrt in {I-1, I}");
            }
            lmmp_free(a); lmmp_free(dstis); lmmp_free(isq);
            lmmp_free(nb); lmmp_free(q);
        }
    }
}

TEST_CASE("numth/cbrt", cbrt_ulong_cbrt_3_nthroot) {
    u64 seed = 0x0badcafef00dfaceull;
    for (int i = 0; i < 1000; ++i) {
        u64 x = xorshift64(seed);
        if (x == 0) x = 1;
        u64 c = icbrt_u64(x);
        TEST_CHECK_EQ(lmmp_cbrt_ulong_(x), c);
        TEST_CHECK_EQ(lmmp_cbrt_chebyshev_(x), c);
        TEST_CHECK_EQ(lmmp_nthroot_ulong_(x, 3), c);
        TEST_CHECK_EQ(lmmp_nthroot_ulong_(x, 2), isqrt_u64(x));
    }
    TEST_CHECK_EQ(lmmp_cbrt_ulong_(1), 1u);
    TEST_CHECK_EQ(lmmp_cbrt_ulong_(UINT64_MAX), (u64)2642245);  // floor(cbrt(2^64-1))

    for (int i = 0; i < 300; ++i) {
        mp_limb_t x[3];
        random_limbs(x, 3, seed, false);
        if (x[1] == 0) x[1] = 1;  // 要求 a1 > 0
        BigInt bx(x, 3);
        u64 c = lmmp_cbrt_3_(x[0], x[1], x[2]);
        BigInt bc(c);
        TEST_CHECK_MSG(BigInt::pow(bc, 3) <= bx && BigInt::pow(BigInt(c + 1), 3) > bx, "cbrt_3 floor property");
    }
}

TEST_CASE("numth/cbrt", cbrt_6) {
    u64 seed = 0x6c62727436746872ull;
    // n ∈ {4,5,6}，覆盖两个分支：顶部归一化（走除法快速路径）与任意归一化（走 log2/exp2 估计路径）
    for (int n : {4, 5, 6}) {
        for (int iter = 0; iter < 300; ++iter) {
            mp_limb_t a[6] = {0, 0, 0, 0, 0, 0};
            random_limbs(a, n, seed, false);
            if (a[n - 1] == 0) a[n - 1] = 1;
            if (iter % 3 == 0) a[n - 1] |= 0x6000000000000000ull;  // 快速路径条件
            if (iter % 3 == 1) a[n - 1] &= 0x1fffffffffffffffull;  // 小顶 limb，估计路径
            BigInt ba(a, n);

            mp_limb_t r[2];
            lmmp_cbrt_6_(r, a, n);
            BigInt br(r, 2);
            BigInt r3 = BigInt::pow(br, 3);
            BigInt rp1 = BigInt::pow(BigInt::add_small(br, 1), 3);
            TEST_CHECK_MSG(r3 <= ba && rp1 > ba, "cbrt_6 floor property");
        }
    }
    // 完全立方数边界（2 limb 底数，立方为 4~6 limb）
    for (int t = 0; t < 50; ++t) {
        BigInt base;
        base.d.resize(2);
        for (size_t i = 0; i < base.d.size(); ++i) base.d[i] = xorshift64(seed);
        base.d[1] &= 0x0000ffffffffffffull;
        base.trim();
        if (base.is_zero()) base = BigInt(1);
        BigInt cube = BigInt::pow(base, 3);
        if (cube.d.size() <= 3 || cube.d.size() > 6) continue;
        mp_limb_t a[6] = {0, 0, 0, 0, 0, 0};
        to_limbs(cube, a, (mp_size_t)cube.d.size());
        BigInt ba(a, (mp_size_t)cube.d.size());
        mp_limb_t r[2];
        lmmp_cbrt_6_(r, a, (mp_size_t)cube.d.size());
        BigInt br(r, 2);
        TEST_CHECK_MSG(BigInt::pow(br, 3) == ba, "cbrt_6 perfect cube");
    }
}

TEST_CASE("numth/cbrt", cbrt_divide) {
    u64 seed = 0x0f15ae2697d3c4b8ull;
    for (mp_size_t ns : {1, 2, 3, 5, 10}) {
        mp_size_t na = 3 * ns;
        mp_ptr numa = alloc_limbs(na);
        mp_ptr dst = alloc_limbs(ns + 1);
        mp_ptr tp = alloc_limbs(4 * ns + 2);
        random_limbs(numa, na, seed);
        numa[na - 1] = (numa[na - 1] & 0x9fffffffffffffull) | 0x6000000000000000ull;
        BigInt bn(numa, na);

        lmmp_cbrt_divide_(dst, numa, ns, tp, 1);
        BigInt bc(dst, ns);
        BigInt rem(numa, 2 * ns + 1);
        BigInt c3 = BigInt::pow(bc, 3);
        TEST_CHECK_MSG(BigInt::add_abs(c3, rem) == bn, "cbrt_divide cbrtrem relation");
        TEST_CHECK_MSG(c3 <= bn && BigInt::pow(BigInt::add_small(bc, 1), 3) > bn, "cbrt_divide floor property");

        lmmp_free(numa); lmmp_free(dst); lmmp_free(tp);
    }
}

TEST_CASE("numth/perfsqr", perfsqr_filter_perfsqr) {
    u64 seed = 0x1122334455667788ull;

    // 完全平方数必须返回 true
    for (size_t n : {1, 2, 3, 5, 8, 17}) {
        BigInt s;
        s.d.resize(n);
        for (size_t i = 0; i < n; ++i) s.d[i] = xorshift64(seed);
        s.trim();
        BigInt sq = BigInt::sqr_school(s);
        mp_ptr p = alloc_limbs(sq.d.size());
        to_limbs(sq, p, (mp_size_t)sq.d.size());
        TEST_CHECK_MSG(lmmp_perfsqr_filter_(p, (mp_size_t)sq.d.size()), "filter accepts square");
        TEST_CHECK_MSG(lmmp_perfsqr_(p, (mp_size_t)sq.d.size()), "perfsqr detects square");
        if (sq.d.size() == 1)
            TEST_CHECK_MSG(lmmp_perfsqr_filter_1_(p[0]), "filter_1 accepts square");
        lmmp_free(p);
    }

    // 小随机数与参考完全平方判断一致
    for (size_t n : {1, 2, 3}) {
        for (int iter = 0; iter < 30; ++iter) {
            BigInt x;
            x.d.resize(n);
            for (size_t i = 0; i < n; ++i) x.d[i] = xorshift64(seed);
            x.trim();
            if (x.is_zero()) continue;
            mp_ptr p = alloc_limbs(x.d.size());
            to_limbs(x, p, (mp_size_t)x.d.size());
            bool expect = ref_perfect_square(x);
            bool got = lmmp_perfsqr_(p, (mp_size_t)x.d.size());
            TEST_CHECK_MSG(got == expect, "perfsqr matches reference");
            bool filt = lmmp_perfsqr_filter_(p, (mp_size_t)x.d.size());
            if (!filt) TEST_CHECK_MSG(!expect, "filter false implies non-square");
            lmmp_free(p);
        }
    }
}

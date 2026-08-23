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

#include "lammp/lmmpn.h"
#include "lammp/impl/mparam.h"
#include "lammp/numth.h"
#include "lammp_test.hpp"
#include "lammp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lammp_test_utils;

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
    u64 seed = 0x3141592653589793ull;
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

        u64 ca = lmmp_cbrtapprox_3_(x[0], x[1], x[2]);
        BigInt bca(ca);
        TEST_CHECK_MSG(BigInt::pow(bca, 3) <= bx && BigInt::pow(BigInt(ca + 2), 3) >= bx, "cbrtapprox_3 approx");
    }
}

TEST_CASE("numth/cbrt", cbrt_divide) {
    u64 seed = 0x0f1e2d3c4b5a6978ull;
    for (mp_size_t ns : {1, 2, 5, 10}) {
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
    for (size_t n : {1, 2, 3, 5, 8}) {
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

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

#include <cstring>
#include <vector>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

BigInt shl1(const BigInt& x) { return BigInt::shl_bits(x, 1); }

}  // namespace

TEST_CASE("low/addsub", add_sub_deterministic_inplace) {
    const mp_size_t n = 5;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);

    const u64 av[5] = {UINT64_MAX - 1, 0, 0, 1, UINT64_MAX};
    const u64 bv[5] = {1, 0, UINT64_MAX, UINT64_MAX - 1, 0};
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    BigInt ba(a, n), bb(b, n);

    // dst == numa
    mp_ptr tmp = alloc_limbs(n);
    std::memcpy(tmp, a, n * 8);
    mp_limb_t cy = lmmp_add_n_(tmp, tmp, b, n);
    BigInt expect = BigInt::add_abs(ba, bb);
    TEST_CHECK_EQ(cy, expect.d.size() > (size_t)n ? expect.d[n] : 0u);
    TEST_CHECK(limb_vec_eq(BigInt(expect.d.data(), (size_t)n), tmp, n));

    // dst == numb
    std::memcpy(tmp, b, n * 8);
    cy = lmmp_add_n_(tmp, a, tmp, n);
    TEST_CHECK_EQ(cy, expect.d.size() > (size_t)n ? expect.d[n] : 0u);
    TEST_CHECK(limb_vec_eq(BigInt(expect.d.data(), (size_t)n), tmp, n));

    // dst == numa，减法（ba > bb 的真差）
    std::memcpy(tmp, a, n * 8);
    cy = lmmp_sub_n_(tmp, tmp, b, n);
    BigInt diff = BigInt::sub_abs(ba, bb);
    TEST_CHECK_EQ(cy, 0u);
    TEST_CHECK(limb_vec_eq(diff, tmp, n));

    // dst == numb，减法（ba > bb 的真差）
    std::memcpy(tmp, b, n * 8);
    cy = lmmp_sub_n_(tmp, a, tmp, n);
    TEST_CHECK_EQ(cy, 0u);
    TEST_CHECK(limb_vec_eq(diff, tmp, n));

    // 同时覆盖借位方向：若 ba < bb，借位为 1，结果为 2^(64n) - (bb - ba)
    std::memcpy(tmp, a, n * 8);
    cy = lmmp_sub_n_(tmp, tmp, b, n);
    if (ba < bb) {
        BigInt powB = BigInt::shl_bits(BigInt(1), n * 64);
        BigInt expect_borrow = BigInt::sub_abs(powB, BigInt::sub_abs(bb, ba));
        TEST_CHECK_EQ(cy, 1u);
        TEST_CHECK(limb_vec_eq(expect_borrow, tmp, n));
    } else {
        TEST_CHECK_EQ(cy, 0u);
        TEST_CHECK(limb_vec_eq(BigInt::sub_abs(ba, bb), tmp, n));
    }

    lmmp_free(a); lmmp_free(b); lmmp_free(tmp);
}

TEST_CASE("low/shift", shlnot_not_inplace_deterministic) {
    const mp_size_t n = 6;
    const u64 vals[6] = {
        0x0000000000000001ull, 0x8000000000000000ull,
        0xffffffffffffffffull, 0x123456789abcdef0ull,
        0x0fedcba987654321ull, 0xaaaaaaaa55555555ull
    };
    mp_ptr a = alloc_limbs(n);
    mp_ptr orig = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) a[i] = vals[i];
    std::memcpy(orig, a, n * 8);
    BigInt ba(a, n);

    lmmp_not_(a, a, n);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(a[i], ~orig[i]);
    lmmp_not_(a, a, n);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(a[i], orig[i]);

    for (mp_size_t shl : {1, 7, 31, 63}) {
        std::memcpy(a, orig, n * 8);
        (void)lmmp_shlnot_(a, a, n, shl);
        BigInt shifted = BigInt::shl_bits(ba, shl);
        for (mp_size_t i = 0; i < n; ++i) {
            u64 s = (i < (mp_size_t)shifted.d.size()) ? shifted.d[i] : 0ull;
            TEST_CHECK_EQ(a[i], ~s);
        }
    }

    lmmp_free(a); lmmp_free(orig);
}

TEST_CASE("low/div", div_1_mod_1_boundary_deterministic) {
    for (mp_size_t n : {1, 2, 3}) {
        for (u64 fill : {0ull, 1ull, UINT64_MAX}) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr q = alloc_limbs(n + 1);
            for (mp_size_t i = 0; i < n; ++i) a[i] = (i == n - 1) ? fill : (fill >> 1);
            BigInt ba(a, n);

            for (u64 x : {1ull, 2ull, 0x8000000000000000ull, UINT64_MAX}) {
                u64 ref_r = BigInt::mod_small(ba, x);
                BigInt ref_q = BigInt::div_small(ba, x, ref_r);

                std::memcpy(q, a, n * 8);
                q[n] = 0;
                u64 r = lmmp_div_1_(q, q, n, x);
                TEST_CHECK_EQ(r, ref_r);
                TEST_CHECK(from_limbs(q, n) == ref_q);

                r = lmmp_div_1_(NULL, a, n, x);
                TEST_CHECK_EQ(r, ref_r);

                r = lmmp_mod_1_(a, n, x);
                TEST_CHECK_EQ(r, ref_r);
            }

            lmmp_free(a); lmmp_free(q);
        }
    }
}

TEST_CASE("low/addsub", addshl1_subshl1_boundary_deterministic) {
    const mp_size_t n = 4;
    const u64 av[4] = {0, 1, UINT64_MAX, 0};
    const u64 bv[4] = {1, 0, UINT64_MAX, UINT64_MAX};
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    BigInt ba(a, n), bb(b, n);

    BigInt sum = BigInt::add_abs(ba, shl1(bb));
    mp_limb_t cy = lmmp_addshl1_n_(d, a, b, n);
    TEST_CHECK_EQ(cy, sum.d.size() > (size_t)n ? sum.d[n] : 0u);
    TEST_CHECK(limb_vec_eq(BigInt(sum.d.data(), (size_t)n), d, n));

    for (mp_size_t i = 0; i < n; ++i) { a[i] = UINT64_MAX; b[i] = 0; }
    BigInt ba2(a, n);
    BigInt diff = BigInt::sub_abs(ba2, shl1(BigInt(b, n)));
    cy = lmmp_subshl1_n_(d, a, b, n);
    TEST_CHECK_EQ(cy, 0u);
    TEST_CHECK(limb_vec_eq(diff, d, n));

    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

TEST_CASE("numth/combin", nPr_nCr_degenerate_boundaries) {
    for (ulong n : {0, 1, 2, 3, 5, 10, 12}) {
        for (ulong r : {0ull, 1ull, n}) {
            if (r > n) continue;
            BigInt expect(1);
            if (r == 0) {
                expect = BigInt(1);
            } else if (r == 1) {
                expect = BigInt(n);
            } else {
                for (ulong i = 2; i <= n; ++i) expect = BigInt::mul_school(expect, BigInt(i));
            }
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nPr_size_(n, r, &bits);
            mp_ptr dst = alloc_limbs((size_t)need + 2);
            mp_size_t rn = lmmp_nPr_(dst, bits, need, n, r);
            TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "nPr degenerate");
            lmmp_free(dst);
        }
    }

    for (uint n : {0, 1, 2, 5, 10, 20}) {
        uint half = n / 2;
        for (uint r : {0u, 1u, half}) {
            if (r > half) continue; /* nCr 要求 r <= n/2 */
            BigInt fn(1), fr(1), fnr(1);
            for (uint i = 2; i <= n; ++i) fn = BigInt::mul_school(fn, BigInt(i));
            for (uint i = 2; i <= r; ++i) fr = BigInt::mul_school(fr, BigInt(i));
            for (uint i = 2; i <= n - r; ++i) fnr = BigInt::mul_school(fnr, BigInt(i));
            BigInt rem;
            BigInt expect = BigInt::divmod_school(fn, BigInt::mul_school(fr, fnr), rem);

            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nCr_size_(n, r, &bits);
            mp_ptr dst = alloc_limbs((size_t)need + 2);
            mp_size_t rn = lmmp_nCr_(dst, bits, need, n, r);
            TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "nCr degenerate");
            lmmp_free(dst);
        }
    }

    {
        ulong n = 0xffff;
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_nPr_size_(n, 1, &bits);
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_nPr_(dst, bits, need, n, 1);
        TEST_CHECK(from_limbs(dst, rn) == BigInt(n));
        lmmp_free(dst);
    }
}


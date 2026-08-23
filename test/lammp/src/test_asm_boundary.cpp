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

#include <cstdio>
#include <cstring>

using namespace lammp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

}  // namespace

TEST_CASE("asm/boundary", add_nc_sub_nc_all_ones) {
    const mp_size_t n = 8;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) {
        a[i] = UINT64_MAX;
        b[i] = 0;
    }
    b[0] = 1;

    mp_limb_t cy = lmmp_add_nc_(d, a, b, n, 1);
    TEST_CHECK_EQ(cy, 1u);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(d[i], (i == 0) ? 1u : 0u);

    // a - a - borrow = all ones, borrow 1
    for (mp_size_t i = 0; i < n; ++i) a[i] = UINT64_MAX;
    mp_limb_t bw = lmmp_sub_nc_(d, a, a, n, 1);
    TEST_CHECK_EQ(bw, 1u);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(d[i], UINT64_MAX);

    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

TEST_CASE("asm/boundary", mul_1_addmul_submul_extremes) {
    const mp_size_t n = 6;
    mp_ptr a = alloc_limbs(n);
    mp_ptr d = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) a[i] = UINT64_MAX;

    BigInt ba(a, n);
    u64 x = UINT64_MAX;
    BigInt prod = BigInt::mul_school(ba, BigInt(x));
    mp_limb_t cy = lmmp_mul_1_(d, a, n, x);
    TEST_CHECK(limb_vec_eq(BigInt(prod.d.data(), n), d, n));
    TEST_CHECK_EQ(cy, prod.d[n]);

    mp_ptr acc = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) acc[i] = UINT64_MAX;
    acc[n] = 0;
    BigInt bacc(acc, n);
    BigInt sum = BigInt::add_abs(bacc, prod);
    u64 c2 = lmmp_addmul_1_(acc, a, n, x);
    TEST_CHECK(limb_vec_eq(BigInt(sum.d.data(), sum.d.size() > n ? n : sum.d.size()), acc, n));
    TEST_CHECK_EQ(c2, sum.d.size() > n ? sum.d[n] : 0u);

    // submul_1：构造足够大的被加数（借位版）仅检查返回 limb 关系
    BigInt accbig(acc, n);
    mp_ptr acc2 = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) acc2[i] = acc[i];
    acc2[n] = 0;
    u64 c3 = lmmp_submul_1_(acc2, a, n, x);
    BigInt prod_low;
    size_t pl = prod.d.size() < n ? prod.d.size() : n;
    prod_low.d.assign(pl, 0);
    for (size_t i = 0; i < pl; ++i) prod_low.d[i] = prod.d[i];
    prod_low.trim();
    u64 expect_borrow = (prod.d.size() > n ? prod.d[n] : 0) + (accbig < prod_low ? 1u : 0u);
    TEST_CHECK_EQ(c3, expect_borrow);

    lmmp_free(a); lmmp_free(d); lmmp_free(acc); lmmp_free(acc2);
}

TEST_CASE("asm/boundary", shl_shr_inplace) {
    u64 seed = 0x1234567890abcdeful;
    const mp_size_t n = 16;
    mp_ptr a = alloc_limbs(n);
    mp_ptr orig = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) a[i] = xorshift64(seed);
    BigInt ba(a, n);
    std::memcpy(orig, a, n * 8);

    for (size_t s : {1, 7, 31, 63}) {
        std::memcpy(a, orig, n * 8);
        BigInt expected = BigInt::shl_bits(ba, s);
        mp_limb_t ret = lmmp_shl_(a, a, n, s);  // 原地左移
        BigInt got = from_limbs(a, n);
        (void)ret;
        TEST_CHECK_MSG(got == BigInt(expected.d.data(), expected.d.size() > n ? n : expected.d.size()),
                       "inplace shl body");

        std::memcpy(a, orig, n * 8);
        BigInt expected_shr = BigInt::shr_bits(ba, s);
        ret = lmmp_shr_(a, a, n, s);  // 原地右移
        got = from_limbs(a, n);
        (void)ret;
        TEST_CHECK_MSG(got == expected_shr, "inplace shr body");
    }

    lmmp_free(a); lmmp_free(orig);
}

TEST_CASE("asm/boundary", div_1_by_one_and_power_of_two) {
    u64 seed = 0xf00dface12345678ull;
    for (mp_size_t n : {1, 2, 5, 16}) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr q = alloc_limbs(n + 1);
        for (mp_size_t i = 0; i < n; ++i) a[i] = xorshift64(seed);
        BigInt ba(a, n);

        // x == 1：商等于原数，余数 0
        std::memcpy(q, a, n * 8);
        u64 r = lmmp_div_1_(q, q, n, 1);
        TEST_CHECK_EQ(r, 0u);
        TEST_CHECK(from_limbs(q, n) == ba);

        // x 为 2 的幂：直接与参考比较
        u64 x = 1ull << (seed & 63 ? (seed & 63) : 1);
        x = (x == 1) ? 2 : x;
        u64 ref_r = BigInt::mod_small(ba, x);
        BigInt ref_q = BigInt::div_small(ba, x, ref_r);
        std::memcpy(q, a, n * 8);
        r = lmmp_div_1_(q, q, n, x);
        TEST_CHECK_EQ(r, ref_r);
        TEST_CHECK(from_limbs(q, n) == ref_q);

        lmmp_free(a); lmmp_free(q);
    }
}

TEST_CASE("asm/boundary", inv_and_mulmod_boundary) {
    u64 seed = 0x0123456789abcdeful;
    const u128 max128(UINT64_MAX, UINT64_MAX);  // 2^128 - 1
    const u128 B = u128(1) << 64;
    for (int i = 0; i < 200; ++i) {
        u64 x = xorshift64(seed) | LIMB_B_2;
        u64 inv = lmmp_inv_1_(x);
        u128 q = (u128)inv + B;
        u128 prod = q * x;
        TEST_CHECK_MSG(prod <= max128 && prod > max128 - x, "inv_1 boundary");

        u64 mod = xorshift64(seed) | 3;
        u64 a = xorshift64(seed) % mod;
        u64 b = xorshift64(seed) % mod;
        u64 qq = 0;
        u64 r = lmmp_mulmod_ulong_(a, b, mod, &qq);
        TEST_CHECK_MSG((u128)qq * mod + r == (u128)a * b, "mulmod boundary relation");
    }
}

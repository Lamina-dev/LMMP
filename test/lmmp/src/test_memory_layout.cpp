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
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void check_low_limbs(const BigInt& full, const mp_limb_t* p, mp_size_t n) {
    const size_t take = full.d.size() < (size_t)n ? full.d.size() : (size_t)n;
    TEST_CHECK(limb_vec_eq(BigInt(full.d.data(), take), p, n));
}

}  // namespace

TEST_CASE("memory/addsub", add_n_sub_n_inplace_pair) {
    const mp_size_t n = 5;
    const u64 av[5] = {UINT64_MAX - 2, 1, 0, UINT64_MAX, 7};
    const u64 bv[5] = {3, UINT64_MAX, UINT64_MAX - 1, 0, 1};

    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    const BigInt ba(a, n);
    const BigInt bb(b, n);
    const BigInt sum = BigInt::add_abs(ba, bb);
    const BigInt diff = BigInt::sub_abs(ba, bb);
    const bool carry = sum.d.size() > (size_t)n;
    const bool borrow = ba < bb;

    /* dsta == numa 且 dstb == numb，同时原地写回两个输入。 */
    const mp_limb_t cb = lmmp_add_n_sub_n_(a, b, a, b, n);
    TEST_CHECK_EQ(cb, (carry ? 2u : 0u) | (borrow ? 1u : 0u));
    check_low_limbs(sum, a, n);
    if (!borrow)
        check_low_limbs(diff, b, n);

    /* 交换两个输出位置：dsta == numb、dstb == numa。 */
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    const mp_limb_t cb2 = lmmp_add_n_sub_n_(b, a, a, b, n);
    TEST_CHECK_EQ(cb2, cb);
    check_low_limbs(sum, b, n);
    if (!borrow)
        check_low_limbs(diff, a, n);

    lmmp_free(a);
    lmmp_free(b);
}

TEST_CASE("memory/addsub", addshl1_subshl1_inplace) {
    const mp_size_t n = 4;
    const u64 av[4] = {1, 0, UINT64_MAX - 1, 5};
    const u64 bv[4] = {UINT64_MAX, 1, 0, 0};

    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    const BigInt ba(a, n);
    const BigInt bb(b, n);
    const BigInt sum = BigInt::add_abs(ba, BigInt::shl_bits(bb, 1));

    for (mp_size_t i = 0; i < n; ++i) a[i] = av[i];
    const mp_limb_t cy = lmmp_addshl1_n_(a, a, b, n);
    check_low_limbs(sum, a, n);
    TEST_CHECK_EQ(cy, sum.d.size() > (size_t)n ? sum.d[n] : 0u);

    /* subshl1：构造 a >= b<<1。 */
    const u64 bigv[4] = {0, 0, 0, 2};
    for (mp_size_t i = 0; i < n; ++i) a[i] = bigv[i];
    const BigInt ba2(a, n);
    const BigInt bshl = BigInt::shl_bits(bb, 1);
    if (ba2 >= bshl) {
        const BigInt diff = BigInt::sub_abs(ba2, bshl);
        const mp_limb_t bw = lmmp_subshl1_n_(b, a, b, n); /* dst == numb */
        check_low_limbs(diff, b, n);
        TEST_CHECK_EQ(bw, 0u);
    } else {
        TEST_CHECK_MSG(false, "test vector precondition");
    }

    lmmp_free(a);
    lmmp_free(b);
}

TEST_CASE("memory/shift", shr1add_shr1sub_inplace) {
    const mp_size_t n = 6;
    const u64 av[6] = {0, UINT64_MAX, 1, UINT64_MAX - 1, 0, 3};
    const u64 bv[6] = {1, 1, UINT64_MAX, 0, UINT64_MAX, 0};

    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    const BigInt ba(a, n);
    const BigInt bb(b, n);
    const BigInt sum = BigInt::add_abs(ba, bb);
    const BigInt half_sum = BigInt::shr_bits(sum, 1);

    for (mp_size_t i = 0; i < n; ++i) a[i] = av[i];
    const mp_limb_t lsb = lmmp_shr1add_n_(a, a, b, n); /* dst == numa */
    check_low_limbs(half_sum, a, n);
    TEST_CHECK_EQ(lsb, sum.d[0] & 1u);

    for (mp_size_t i = 0; i < n; ++i) b[i] = bv[i];
    const mp_limb_t lsb2 = lmmp_shr1add_n_(b, a, b, n); /* dst == numb，a 为上次结果 */
    /* 上面第二个调用使用已被覆盖的 a，这里仅检查不越界/返回位宽。 */
    TEST_CHECK(lsb2 <= 1u);

    /* 重新构造，验证 dst == numb 的 shr1add 正确性。 */
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    const mp_limb_t lsb3 = lmmp_shr1add_n_(b, a, b, n);
    check_low_limbs(half_sum, b, n);
    TEST_CHECK_EQ(lsb3, sum.d[0] & 1u);

    /* shr1sub：a >= b 时 dst 原地覆盖 a 或 b。 */
    const BigInt diff = BigInt::sub_abs(ba, bb);
    const BigInt half_diff = BigInt::shr_bits(diff, 1);
    if (ba >= bb) {
        for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
        const mp_limb_t lsb_s = lmmp_shr1sub_n_(a, a, b, n);
        check_low_limbs(half_diff, a, n);
        TEST_CHECK_EQ(lsb_s, diff.d[0] & 1u);

        for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
        const mp_limb_t lsb_s2 = lmmp_shr1sub_n_(b, a, b, n);
        check_low_limbs(half_diff, b, n);
        TEST_CHECK_EQ(lsb_s2, diff.d[0] & 1u);
    }

    lmmp_free(a);
    lmmp_free(b);
}

TEST_CASE("memory/mul1", mul_1_shifted_overlap) {
    const mp_size_t n = 5;
    const u64 av[5] = {0, 1, UINT64_MAX, 0x8000000000000001ull, 0};
    const u64 x = UINT64_C(0x9e3779b97f4a7c15);

    mp_ptr a = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) a[i] = av[i];
    const BigInt ba(a, n);
    const BigInt prod = BigInt::mul_school(ba, BigInt(x));

    /* dst == numa。 */
    mp_ptr buf = alloc_limbs(n + 2);
    std::memcpy(buf, a, n * sizeof(mp_limb_t));
    const mp_limb_t c0 = lmmp_mul_1_(buf, buf, n, x);
    check_low_limbs(prod, buf, n);
    TEST_CHECK_EQ(c0, prod.d.size() > (size_t)n ? prod.d[n] : 0u);

    /* 极端 n==1 同样允许两种布局。 */
    buf[0] = UINT64_MAX;
    buf[1] = 0;
    buf[2] = 0;
    const mp_limb_t c2 = lmmp_mul_1_(buf + 1, buf, 1, UINT64_MAX);
    TEST_CHECK_EQ(buf[1], 1u);             /* 2^128 - 2^65 + 1 的低 limb */
    TEST_CHECK_EQ(c2, UINT64_MAX - 1);     /* 高 limb */
    TEST_CHECK_EQ(buf[2], 0u);

    lmmp_free(a);
    lmmp_free(buf);
}

TEST_CASE("memory/div1", div_1_shifted_overlap) {
    const mp_size_t n = 5;
    const u64 av[5] = {0, 0, UINT64_MAX, 1, 0x7fffffffffffffffull};
    const u64 x = UINT64_C(0x100000001b3);

    mp_ptr a = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) a[i] = av[i];
    const BigInt ba(a, n);
    u64 ref_r = 0;
    const BigInt ref_q = BigInt::div_small(ba, x, ref_r);

    mp_ptr buf = alloc_limbs(n + 1);

    /* dstq == numa。 */
    std::memcpy(buf + 1, a, n * sizeof(mp_limb_t));
    const mp_limb_t r0 = lmmp_div_1_(buf + 1, buf + 1, n, x);
    TEST_CHECK_EQ(r0, ref_r);
    TEST_CHECK(from_limbs(buf + 1, n) == ref_q);

    /* dstq == numa - 1，即库允许的左移一格重叠布局。 */
    std::memcpy(buf + 1, a, n * sizeof(mp_limb_t));
    const mp_limb_t r1 = lmmp_div_1_(buf, buf + 1, n, x);
    TEST_CHECK_EQ(r1, ref_r);
    TEST_CHECK(from_limbs(buf, n) == ref_q);

    lmmp_free(a);
    lmmp_free(buf);
}

TEST_CASE("memory/muladd", addmul_submul_equal_sources) {
    const mp_size_t n = 4;
    const u64 av[4] = {UINT64_MAX - 1, 0, 1, 7};
    const u64 x = 3;

    mp_ptr a = alloc_limbs(n);
    mp_ptr backup = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) a[i] = av[i];
    const BigInt ba(a, n);

    /* addmul_1 允许 numa == numb：a <- a + a*x = a*(1+x)。 */
    const BigInt expected_add = BigInt::mul_school(ba, BigInt(1 + x));
    std::memcpy(backup, a, n * sizeof(mp_limb_t));
    const mp_limb_t c_add = lmmp_addmul_1_(a, a, n, x);
    check_low_limbs(expected_add, a, n);
    TEST_CHECK_EQ(c_add, expected_add.d.size() > (size_t)n ? expected_add.d[n] : 0u);

    /* submul_1 允许 numa == numb：a <- a - a*x；x==1 时结果为零且无借位。 */
    std::memcpy(a, backup, n * sizeof(mp_limb_t));
    const mp_limb_t c_sub_zero = lmmp_submul_1_(a, a, n, 1);
    TEST_CHECK_EQ(c_sub_zero, 0u);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(a[i], 0u);

    /* x==2 时验证借位关系：a - 2a 的低 n limbs 与完整借位。 */
    std::memcpy(a, backup, n * sizeof(mp_limb_t));
    const BigInt prod = BigInt::mul_school(ba, BigInt(2));
    BigInt prod_low;
    const size_t take = prod.d.size() < (size_t)n ? prod.d.size() : (size_t)n;
    prod_low.d.assign(take, 0);
    for (size_t i = 0; i < take; ++i) prod_low.d[i] = prod.d[i];
    prod_low.trim();
    const u64 expect_borrow = (prod.d.size() > (size_t)n ? prod.d[n] : 0u)
                              + (ba < prod_low ? 1u : 0u);
    const mp_limb_t c_sub = lmmp_submul_1_(a, a, n, 2);
    TEST_CHECK_EQ(c_sub, expect_borrow);

    lmmp_free(a);
    lmmp_free(backup);
}

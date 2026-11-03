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
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

/*
    内存布局（原地/错位重叠）专项：算术正确性由 low/asm 系列覆盖，
    这里以「分离缓冲的同函数调用」为期望，验证重叠布局不破坏结果、
    不越界写。
*/

TEST_CASE("memory/addsub", add_n_sub_n_inplace_pair) {
    const mp_size_t n = 5;
    const u64 av[5] = {UINT64_MAX - 2, 1, 0, UINT64_MAX, 7};
    const u64 bv[5] = {3, UINT64_MAX, UINT64_MAX - 1, 0, 1};

    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), sa = alloc_limbs(n), sb = alloc_limbs(n),
           ref_sum = alloc_limbs(n), ref_diff = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) {
        a[i] = av[i]; b[i] = bv[i]; sa[i] = av[i]; sb[i] = bv[i];
    }
    u64 carry = lmmp_add_n_(ref_sum, sa, sb, n);
    u64 borrow = lmmp_sub_n_(ref_diff, sa, sb, n);

    /* dsta == numa 且 dstb == numb，同时原地写回两个输入。 */
    const mp_limb_t cb = lmmp_add_n_sub_n_(a, b, a, b, n);
    TEST_CHECK_EQ(cb, (carry ? 2u : 0u) | (borrow ? 1u : 0u));
    TEST_CHECK_EQ(memcmp(a, ref_sum, n * 8), 0);
    TEST_CHECK_EQ(memcmp(b, ref_diff, n * 8), 0);

    /* 交换两个输出位置：dsta == numb、dstb == numa。 */
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    const mp_limb_t cb2 = lmmp_add_n_sub_n_(b, a, a, b, n);
    TEST_CHECK_EQ(cb2, cb);
    TEST_CHECK_EQ(memcmp(b, ref_sum, n * 8), 0);
    TEST_CHECK_EQ(memcmp(a, ref_diff, n * 8), 0);

    lmmp_free(a); lmmp_free(b); lmmp_free(sa); lmmp_free(sb);
    lmmp_free(ref_sum); lmmp_free(ref_diff);
}

TEST_CASE("memory/addsub", addshl1_subshl1_inplace) {
    const mp_size_t n = 4;
    const u64 av[4] = {1, 0, UINT64_MAX - 1, 5};
    const u64 bv[4] = {UINT64_MAX, 1, 0, 0};

    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), sa = alloc_limbs(n), sb = alloc_limbs(n),
           ref = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) {
        a[i] = av[i]; b[i] = bv[i]; sa[i] = av[i]; sb[i] = bv[i];
    }

    /* dst == numa（原地）。 */
    u64 cy_ref = lmmp_addshl1_n_(ref, sa, sb, n);
    const mp_limb_t cy = lmmp_addshl1_n_(a, a, b, n);
    TEST_CHECK_EQ(cy, cy_ref);
    TEST_CHECK_EQ(memcmp(a, ref, n * 8), 0);

    /* subshl1：a = [0,0,0,2] >= b<<1，dst == numb。 */
    const u64 bigv[4] = {0, 0, 0, 2};
    for (mp_size_t i = 0; i < n; ++i) { a[i] = bigv[i]; sa[i] = bigv[i]; sb[i] = bv[i]; }
    u64 bw_ref = lmmp_subshl1_n_(ref, sa, sb, n);
    const mp_limb_t bw = lmmp_subshl1_n_(b, a, b, n);
    TEST_CHECK_EQ(bw, bw_ref);
    TEST_CHECK_EQ(memcmp(b, ref, n * 8), 0);

    lmmp_free(a); lmmp_free(b); lmmp_free(sa); lmmp_free(sb); lmmp_free(ref);
}

TEST_CASE("memory/shift", shr1add_shr1sub_inplace) {
    const mp_size_t n = 6;
    const u64 av[6] = {0, UINT64_MAX, 1, UINT64_MAX - 1, 0, 3};
    const u64 bv[6] = {1, 1, UINT64_MAX, 0, UINT64_MAX, 0};

    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), sa = alloc_limbs(n), sb = alloc_limbs(n),
           ref = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) {
        a[i] = av[i]; b[i] = bv[i]; sa[i] = av[i]; sb[i] = bv[i];
    }

    /* dst == numa。 */
    u64 l_ref = lmmp_shr1add_n_(ref, sa, sb, n);
    const mp_limb_t lsb = lmmp_shr1add_n_(a, a, b, n);
    TEST_CHECK_EQ(lsb, l_ref);
    TEST_CHECK_EQ(memcmp(a, ref, n * 8), 0);

    /* dst == numb。 */
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    l_ref = lmmp_shr1add_n_(ref, sa, sb, n);
    const mp_limb_t lsb3 = lmmp_shr1add_n_(b, a, b, n);
    TEST_CHECK_EQ(lsb3, l_ref);
    TEST_CHECK_EQ(memcmp(b, ref, n * 8), 0);

    /* shr1sub：a >= b 时 dst 原地覆盖 a 或 b（先恢复 a/b——上一段
       dst==numb 的 shr1add 已改写 b）。 */
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
    if (mpn_cmp(sa, n, sb, n) >= 0) {
        l_ref = lmmp_shr1sub_n_(ref, sa, sb, n);
        const mp_limb_t lsb_s = lmmp_shr1sub_n_(a, a, b, n);
        TEST_CHECK_EQ(lsb_s, l_ref);
        TEST_CHECK_EQ(memcmp(a, ref, n * 8), 0);

        for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }
        const mp_limb_t lsb_s2 = lmmp_shr1sub_n_(b, a, b, n);
        TEST_CHECK_EQ(lsb_s2, l_ref);
        TEST_CHECK_EQ(memcmp(b, ref, n * 8), 0);
    }

    lmmp_free(a); lmmp_free(b); lmmp_free(sa); lmmp_free(sb); lmmp_free(ref);
}

TEST_CASE("memory/mul1", mul_1_shifted_overlap) {
    const mp_size_t n = 5;
    const u64 av[5] = {0, 1, UINT64_MAX, 0x8000000000000001ull, 0};
    const u64 x = UINT64_C(0x9e3779b97f4a7c15);

    mp_ptr a = alloc_limbs(n), buf = alloc_limbs(n + 2), ref = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) a[i] = av[i];

    /* dst == numa；期望取分离调用。 */
    memcpy(buf, a, n * sizeof(mp_limb_t));
    u64 c_ref = lmmp_mul_1_(ref, a, n, x);
    const mp_limb_t c0 = lmmp_mul_1_(buf, buf, n, x);
    TEST_CHECK_EQ(c0, c_ref);
    TEST_CHECK_EQ(memcmp(buf, ref, n * 8), 0);

    /* 极端 n==1：写死常数 (2^64-1)^2 = 2^128 - 2^65 + 1。 */
    buf[0] = UINT64_MAX;
    buf[1] = 0;
    buf[2] = 0;
    const mp_limb_t c2 = lmmp_mul_1_(buf + 1, buf, 1, UINT64_MAX);
    TEST_CHECK_EQ(buf[1], 1u);          /* 低 limb */
    TEST_CHECK_EQ(c2, UINT64_MAX - 1);  /* 高 limb */
    TEST_CHECK_EQ(buf[2], 0u);

    lmmp_free(a);
    lmmp_free(buf);
    lmmp_free(ref);
}

TEST_CASE("memory/div1", div_1_shifted_overlap) {
    const mp_size_t n = 5;
    const u64 av[5] = {0, 0, UINT64_MAX, 1, 0x7fffffffffffffffull};
    const u64 x = UINT64_C(0x100000001b3);

    mp_ptr a = alloc_limbs(n), buf = alloc_limbs(n + 1), refq = alloc_limbs(n + 1),
           t = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) a[i] = av[i];

    /* 分离参考（另加重建恒等式 q*x + r == a）。 */
    u64 r_ref = lmmp_div_1_(refq, a, n, x);
    u64 hi = lmmp_mul_1_(t, refq, n, x);
    t[n] = hi;
    add_small_inplace(t, n + 1, r_ref);
    TEST_CHECK(mpn_eq(t, n + 1, a, n));

    /* dstq == numa。 */
    memcpy(buf + 1, a, n * sizeof(mp_limb_t));
    const mp_limb_t r0 = lmmp_div_1_(buf + 1, buf + 1, n, x);
    TEST_CHECK_EQ(r0, r_ref);
    TEST_CHECK_EQ(memcmp(buf + 1, refq, n * 8), 0);

    /* dstq == numa - 1，即库允许的左移一格重叠布局。 */
    memcpy(buf + 1, a, n * sizeof(mp_limb_t));
    const mp_limb_t r1 = lmmp_div_1_(buf, buf + 1, n, x);
    TEST_CHECK_EQ(r1, r_ref);
    TEST_CHECK_EQ(memcmp(buf, refq, n * 8), 0);

    lmmp_free(a);
    lmmp_free(buf);
    lmmp_free(refq);
    lmmp_free(t);
}

TEST_CASE("memory/muladd", addmul_submul_equal_sources) {
    const mp_size_t n = 4;
    const u64 av[4] = {UINT64_MAX - 1, 0, 1, 7};
    const u64 x = 3;

    mp_ptr a = alloc_limbs(n), backup = alloc_limbs(n), t = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) a[i] = av[i];

    /* addmul_1 允许 numa == numb：期望 = a + a*x（mul_1 + add_n 组合）。
       sum 低 n limbs 在 t，sum 第 n limb = hi + add_n 进位。 */
    memcpy(backup, a, n * sizeof(mp_limb_t));
    u64 hi = lmmp_mul_1_(t, backup, n, x);
    u64 c = lmmp_add_n_(t, backup, t, n);
    u64 sum_top = hi + c;  // addmul 进位输出
    const mp_limb_t c_add = lmmp_addmul_1_(a, a, n, x);
    TEST_CHECK_EQ(memcmp(a, t, n * 8), 0);
    TEST_CHECK_EQ(c_add, sum_top);

    /* submul_1 允许 numa == numb：x==1 时结果为零且无借位。 */
    memcpy(a, backup, n * sizeof(mp_limb_t));
    const mp_limb_t c_sub_zero = lmmp_submul_1_(a, a, n, 1);
    TEST_CHECK_EQ(c_sub_zero, 0u);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(a[i], 0u);

    /* x==2：a - 2a 低 n limbs 全零、借位 = 顶 limb+1 语义（组合路径复核）。 */
    memcpy(a, backup, n * sizeof(mp_limb_t));
    mp_ptr b2 = alloc_limbs(n);
    u64 c1 = lmmp_add_n_(b2, backup, backup, n);  // 2a
    u64 w = lmmp_sub_n_(t, backup, b2, n);        // a - 2a
    const mp_limb_t c_sub = lmmp_submul_1_(a, a, n, 2);
    TEST_CHECK_EQ(memcmp(a, t, n * 8), 0);
    TEST_CHECK_EQ(c_sub, w + c1);

    lmmp_free(a);
    lmmp_free(backup);
    lmmp_free(t);
    lmmp_free(b2);
}

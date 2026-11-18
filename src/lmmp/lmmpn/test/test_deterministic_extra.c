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

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "lmmp/numth.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

/*
    确定性输入回归：期望值取同函数分离缓冲调用（原地/边界形态专项，
    算术正确性由 low/asm 系列覆盖）；div 边界用重建恒等式；
    退化组合数用已知值与 factorial 交叉。
*/

TEST_CASE("low/addsub", add_sub_deterministic_inplace) {
    const mp_size_t n = 5;
    const u64 av[5] = {UINT64_MAX - 1, 0, 0, 1, UINT64_MAX};
    const u64 bv[5] = {1, 0, UINT64_MAX, UINT64_MAX - 1, 0};
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n), r = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }

    mp_ptr a2 = alloc_limbs(n), b2 = alloc_limbs(n);
    const u64 add_cy = lmmp_add_n_(r, a, b, n);   // 分离缓冲参考：和
    const u64 sub_bw = lmmp_sub_n_(d, a, b, n);   // 分离缓冲参考：差（a >= b）
    TEST_CHECK_EQ(add_cy, 0u);  // 固定输入钉住参考语义：无终位进位/借位
    TEST_CHECK_EQ(sub_bw, 0u);
    // eqsep 原地：dst == numa / dst == numb 各两向（返回值与分离缓冲一致）
    memcpy(a2, a, n * 8);
    TEST_CHECK_EQ(lmmp_add_n_(a2, a2, b, n), add_cy);
    TEST_CHECK_EQ(memcmp(a2, r, n * 8), 0);
    memcpy(b2, b, n * 8);
    TEST_CHECK_EQ(lmmp_add_n_(b2, a, b2, n), add_cy);
    TEST_CHECK_EQ(memcmp(b2, r, n * 8), 0);
    memcpy(a2, a, n * 8);
    TEST_CHECK_EQ(lmmp_sub_n_(a2, a2, b, n), sub_bw);
    TEST_CHECK_EQ(memcmp(a2, d, n * 8), 0);
    memcpy(b2, b, n * 8);
    TEST_CHECK_EQ(lmmp_sub_n_(b2, a, b2, n), sub_bw);
    TEST_CHECK_EQ(memcmp(b2, d, n * 8), 0);

    lmmp_free(a); lmmp_free(b); lmmp_free(d); lmmp_free(r);
    lmmp_free(a2); lmmp_free(b2);
}

TEST_CASE("low/shift", shlnot_not_inplace_deterministic) {
    const mp_size_t n = 6;
    const u64 vals[6] = {
        0x0000000000000001ull, 0x8000000000000000ull,
        0xffffffffffffffffull, 0x123456789abcdef0ull,
        0x0fedcba987654321ull, 0xaaaaaaaa55555555ull
    };
    mp_ptr a = alloc_limbs(n), orig = alloc_limbs(n), r = alloc_limbs(n);
    for (mp_size_t i = 0; i < n; ++i) a[i] = vals[i];
    memcpy(orig, a, n * 8);

    lmmp_not_(a, a, n);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(a[i], ~orig[i]);
    lmmp_not_(a, a, n);
    for (mp_size_t i = 0; i < n; ++i) TEST_CHECK_EQ(a[i], orig[i]);

    const mp_size_t s_values[] = {0, 1, 7, 31, 63};
    for (size_t s_index = 0; s_index < ARRAY_SIZE(s_values); ++s_index) {
        mp_size_t s = s_values[s_index];
        mp_limb_t r1 = lmmp_shlnot_(r, orig, n, s);
        memcpy(a, orig, n * 8);
        mp_limb_t r2 = lmmp_shlnot_(a, a, n, s);
        TEST_CHECK_EQ(r1, r2);
        TEST_CHECK_EQ(memcmp(a, r, n * 8), 0);
    }
    lmmp_free(a); lmmp_free(orig); lmmp_free(r);
}

TEST_CASE("low/div", div_1_mod_1_boundary_deterministic) {
    const mp_size_t ns[] = {1, 2, 3};
    const u64 fills[] = {0ull, 1ull, ~0ull};
    const u64 divisors[] = {1ull, 2ull, 0x8000000000000000ull, ~0ull};
    for (size_t n_index = 0; n_index < ARRAY_SIZE(ns); ++n_index) {
        mp_size_t n = ns[n_index];
        for (size_t fill_index = 0; fill_index < ARRAY_SIZE(fills); ++fill_index) {
            u64 fill = fills[fill_index];
            mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), t = alloc_limbs(n + 2);
            for (mp_size_t i = 0; i < n; ++i) a[i] = (i == n - 1) ? fill : (fill >> 1);
            for (size_t x_index = 0; x_index < ARRAY_SIZE(divisors); ++x_index) {
                u64 x = divisors[x_index];
                u64 rem = lmmp_div_1_(q, a, n, x);
                TEST_CHECK(rem < x);
                TEST_CHECK_EQ(lmmp_mod_1_(a, n, x), rem);
                u64 hi = lmmp_mul_1_(t, q, n, x);
                t[n] = hi;
                add_small_inplace(t, n + 1, rem);
                TEST_CHECK(mpn_eq(t, n + 1, a, n));  // q*x + r == a
            }
            lmmp_free(a); lmmp_free(q); lmmp_free(t);
        }
    }
}

TEST_CASE("low/addsub", addshl1_subshl1_boundary_deterministic) {
    const mp_size_t n = 4;
    const u64 av[4] = {0, 1, ~(u64)0 - 1, 3};
    const u64 bv[4] = {(u64)1 << 63, ~(u64)0, 0, 2};
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(n + 1), t = alloc_limbs(n + 1),
           b2 = alloc_limbs(n + 1);
    for (mp_size_t i = 0; i < n; ++i) { a[i] = av[i]; b[i] = bv[i]; }

    u64 c1 = lmmp_add_n_(b2, b, b, n);
    u64 c2 = lmmp_add_n_(t, a, b2, n);
    u64 cy = lmmp_addshl1_n_(d, a, b, n);
    TEST_CHECK_EQ(memcmp(d, t, n * 8), 0);
    TEST_CHECK_EQ(cy, c1 + c2);

    u64 w1 = lmmp_sub_n_(t, a, b2, n);
    u64 bw = lmmp_subshl1_n_(d, a, b, n);
    TEST_CHECK_EQ(memcmp(d, t, n * 8), 0);
    TEST_CHECK_EQ(bw, w1 + c1);

    lmmp_free(a); lmmp_free(b); lmmp_free(d); lmmp_free(t); lmmp_free(b2);
}

void test_deterministic_extra(void) {
    TEST_RUN("low/addsub", add_sub_deterministic_inplace);
    TEST_RUN("low/shift", shlnot_not_inplace_deterministic);
    TEST_RUN("low/div", div_1_mod_1_boundary_deterministic);
    TEST_RUN("low/addsub", addshl1_subshl1_boundary_deterministic);
}

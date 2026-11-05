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
#include "lmmp/impl/mparam.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

// 恒等式重建：q*d + r == n 且 r < d（乘法/加法全用库内路径）
void verify_div_relation(const mp_limb_t* n, mp_size_t na, const mp_limb_t* d, mp_size_t nb,
                         const mp_limb_t* q, mp_size_t qn, const mp_limb_t* r, mp_size_t rn) {
    TEST_CHECK_MSG(mpn_cmp(r, rn, d, nb) < 0, "r < d");

    mp_ptr t = alloc_limbs(na + 1);
    lmmp_mul_(t, q, qn, d, nb);  // t = q*d（qn >= nb 由调用方保证）
    // t += r：r 零垫到 na+1 长后 add_n
    mp_ptr rpad = alloc_limbs(na + 1);
    memset(rpad, 0, (size_t)(na + 1) * 8);
    memcpy(rpad, r, (size_t)rn * 8);
    lmmp_add_n_(t, t, rpad, na + 1);
    TEST_CHECK_MSG(mpn_eq(t, na + 1, n, na), "q*d+r == n");
    lmmp_free(t);
    lmmp_free(rpad);
}

}  // namespace

TEST_CASE("div/single", div_1_mod_1) {
    u64 seed = 0x3141592653589793ull;
    for (size_t n = 1; n <= 80; ++n) {
        for (int iter = 0; iter < 10; ++iter) {
            mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), t = alloc_limbs(n + 1),
                   acopy = alloc_limbs(n);
            random_limbs(a, n, seed);
            u64 x = xorshift64(seed) | 1;

            u64 rem = lmmp_mod_1_(a, n, x);
            TEST_CHECK(rem < x);
            u64 ret_rem = lmmp_div_1_(q, a, n, x);
            TEST_CHECK_EQ(ret_rem, rem);

            // 重建：q*x + rem == a
            u64 hi = lmmp_mul_1_(t, q, n, x);
            t[n] = hi;
            add_small_inplace(t, n + 1, rem);
            TEST_CHECK(mpn_eq(t, n + 1, a, n));

            // div_1_s: 归一化单精度除法（x 顶位为 1 档），结果与 div_1 一致
            if (n > 1 && x >= LIMB_B_2) {
                memcpy(acopy, a, n * 8);
                u64 qh = lmmp_div_1_s_(q, acopy, n, x);
                TEST_CHECK_EQ(acopy[0], rem);
                TEST_CHECK_EQ(qh, q[n - 1]);  // 商顶 limb
                // 商低 n-1 limbs 与 div_1 一致（div_1 已由重建验证）
                mp_ptr q2 = alloc_limbs(n + 1);
                lmmp_div_1_(q2, a, n, x);
                TEST_CHECK_EQ(memcmp(q, q2, (n - 1) * 8), 0);
                lmmp_free(q2);
            }

            lmmp_free(a); lmmp_free(q); lmmp_free(t); lmmp_free(acopy);
        }
    }
}

TEST_CASE("div/general", div_general_relation) {
    u64 seed = 0x2718281828459045ull;
    const mp_size_t sizes[][2] = {
        {2, 1}, {3, 1}, {5, 2}, {10, 3}, {20, 5}, {30, 10},
        {50, 10}, {80, 20}, {120, 30}, {200, 50}, {350, 80},
        {600, 120}, {900, 200}, {1400, 300}, {1900, 400}, {2500, 500}
    };
    for (auto c : sizes) {
        mp_size_t na = c[0], nb = c[1];
        mp_ptr n = alloc_limbs(na), d = alloc_limbs(nb);
        mp_ptr q = alloc_limbs(na - nb + 1), r = alloc_limbs(nb);
        random_limbs(n, na, seed);
        random_limbs(d, nb, seed);  // MSB=1，除数归一化

        lmmp_div_(q, r, n, na, d, nb);
        verify_div_relation(n, na, d, nb, q, na - nb + 1, r, nb);

        lmmp_free(n); lmmp_free(d); lmmp_free(q); lmmp_free(r);
    }
}

TEST_CASE("div/normalized", div_s_normalized_relation) {
    u64 seed = 0x0badcafef00dfaceull;
    const mp_size_t sizes[][2] = {
        {3, 1}, {4, 2}, {10, 3}, {30, 10}, {60, 20}, {120, 40},
        {300, 80}, {700, 150}, {1200, 300}, {1900, 400}
    };
    for (auto c : sizes) {
        mp_size_t na = c[0], nb = c[1];
        mp_ptr numa = alloc_limbs(na), numb = alloc_limbs(nb), nsave = alloc_limbs(na);
        mp_ptr q = alloc_limbs(na - nb + 2);
        random_limbs(numa, na, seed);
        random_limbs(numb, nb, seed);  // MSB=1，已归一化
        memcpy(nsave, numa, (size_t)na * 8);

        mp_limb_t qh = lmmp_div_s_(q, numa, na, numb, nb);
        // 完整商 = q[0..na-nb-1] ++ [qh]，余数 = [numa, nb]
        q[na - nb] = qh;
        verify_div_relation(nsave, na, numb, nb, q, na - nb + 1, numa, nb);

        lmmp_free(numa); lmmp_free(numb); lmmp_free(nsave); lmmp_free(q);
    }
}

TEST_CASE("div/basecase_divide", div_basecase_vs_divide) {
    u64 seed = 0x1020304050607080ull;
    const mp_size_t sizes[][2] = {
        {6, 3}, {10, 4}, {20, 6}, {40, 10}, {60, 20}, {100, 30}, {160, 40}, {220, 50}
    };
    for (auto c : sizes) {
        mp_size_t na = c[0], nb = c[1];
        if (na < nb + 1) continue;
        mp_ptr numa1 = alloc_limbs(na);
        mp_ptr numa2 = alloc_limbs(na);
        mp_ptr numb = alloc_limbs(nb);
        mp_ptr q1 = alloc_limbs(na - nb + 1);
        mp_ptr q2 = alloc_limbs(na - nb + 1);
        random_limbs(numa1, na, seed);
        random_limbs(numb, nb, seed);
        std::memcpy(numa2, numa1, na * 8);

        mp_limb_t inv21 = lmmp_inv_2_1_(numb[nb - 1], numb[nb - 2]);
        mp_limb_t qh1 = lmmp_div_basecase_(q1, numa1, na, numb, nb, inv21);

        // basecase 自身恒等式（全部尺寸；小除数档无 divide 对照时是唯一验证）
        {
            q1[na - nb] = qh1;
            verify_div_relation(numa2, na, numb, nb, q1, na - nb + 1, numa1, nb);
        }

        // div_divide_ 的前置条件为 nb>=6 且 na>=2*nb。
        if (nb >= 6 && na >= 2 * nb) {
            mp_limb_t qh2 = lmmp_div_divide_(q2, numa2, na, numb, nb, inv21);
            TEST_CHECK_EQ(qh1, qh2);
            for (mp_size_t i = 0; i < na - nb; ++i) TEST_CHECK_EQ(q1[i], q2[i]);
            for (mp_size_t i = 0; i < nb; ++i) TEST_CHECK_EQ(numa1[i], numa2[i]);
        }

        lmmp_free(numa1); lmmp_free(numa2); lmmp_free(numb); lmmp_free(q1); lmmp_free(q2);
    }
}

/*
    inv_1 契约：inv = floor((B^2-1)/x) - B，即 x*(B+inv) ∈ (B^2-1-x, B^2-1]。
    积经 mul_1 双 limb 折算（x*B 并入高半，无溢出）。
*/
TEST_CASE("div/inv", inv_1_inv_2_1) {
    u64 seed = 0x0f0e0d0c0b0a0908ull;

    for (int i = 0; i < 500; ++i) {
        u64 x = xorshift64(seed) | LIMB_B_2;  // inv_1_ 要求 MSB(x)=1
        u64 inv = lmmp_inv_1_(x);
        mp_limb_t prod[2];
        u64 hi = lmmp_mul_1_(prod, &x, 1, inv);
        prod[1] = hi;
        u64 p_hi = prod[1] + x;
        TEST_CHECK_EQ(p_hi, ~(u64)0);
        TEST_CHECK(prod[0] >= (u64)(0 - x));
    }

    // inv_2_1 与 div_basecase 的交叉验证已在上一用例覆盖；此处仅非零检查。
    for (int i = 0; i < 200; ++i) {
        u64 xh = xorshift64(seed) | LIMB_B_2;
        u64 xl = xorshift64(seed);
        mp_limb_t inv = lmmp_inv_2_1_(xh, xl);
        TEST_CHECK_MSG(inv != 0, "inv_2_1 nonzero");
    }
}

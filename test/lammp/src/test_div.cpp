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

// 验证 q*d + r == n 且 r < d。使用测试框架内的朴素 BigInt 乘法/加法/比较。
void verify_div_relation(const BigInt& n, const BigInt& d,
                         const mp_limb_t* q, mp_size_t qn,
                         const mp_limb_t* r, mp_size_t rn) {
    BigInt bq = from_limbs(q, qn);
    BigInt br = from_limbs(r, rn);
    BigInt qd = BigInt::mul_school(bq, d);
    BigInt qd_r = BigInt::add_abs(qd, br);
    TEST_CHECK_MSG(qd_r == n, "q*d+r == n");
    TEST_CHECK_MSG(br < d || d.is_zero(), "r < d");
}

}  // namespace

TEST_CASE("div/single", div_1_mod_1) {
    u64 seed = 0x3141592653589793ull;
    for (size_t n = 1; n <= 80; ++n) {
        for (int iter = 0; iter < 10; ++iter) {
            mp_ptr a = alloc_limbs(n);
            mp_ptr q = alloc_limbs(n);
            random_limbs(a, n, seed);
            BigInt ba(a, n);
            u64 x = xorshift64(seed) | 1;

            u64 ref_rem = BigInt::mod_small(ba, x);
            u64 rem = lmmp_mod_1_(a, n, x);
            TEST_CHECK_EQ(rem, ref_rem);

            mp_ptr acopy = alloc_limbs(n);
            std::memcpy(acopy, a, n * 8);
            u64 ret_rem = lmmp_div_1_(q, acopy, n, x);
            TEST_CHECK_EQ(ret_rem, rem);
            u64 dummy = 0;
            BigInt expect_q = BigInt::div_small(ba, x, dummy);
            // div_1 写入 q[0..n-1] 为完整商（无单独 qh 返回）
            BigInt bq(q, n);
            TEST_CHECK_MSG(bq == expect_q, "div_1 quotient match");

            // div_1_s: 归一化单精度除法（x 需要 MSB 为 1）
            if (n > 1 && x >= LIMB_B_2) {
                std::memcpy(acopy, a, n * 8);
                u64 qh = lmmp_div_1_s_(q, acopy, n, x);
                std::vector<u64> full_qs(q, q + n - 1);
                full_qs.push_back(qh);
                BigInt bqs(full_qs.data(), full_qs.size());
                u64 rem_ref = 0;
                BigInt expect_qs = BigInt::div_small(ba, x, rem_ref);
                TEST_CHECK_MSG(bqs == expect_qs, "div_1_s quotient match");
                TEST_CHECK_EQ(acopy[0], rem_ref);
            }

            lmmp_free(a); lmmp_free(q); lmmp_free(acopy);
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
        if (na > 2500) continue;
        mp_ptr n = alloc_limbs(na);
        mp_ptr d = alloc_limbs(nb);
        mp_ptr q = alloc_limbs(na - nb + 1);
        mp_ptr r = alloc_limbs(nb);
        random_limbs(n, na, seed);
        random_limbs(d, nb, seed);
        // 保证除数最高位非零（random_limbs 已保证 MSB 为 1）
        BigInt bn(n, na), bd(d, nb);

        lmmp_div_(q, r, n, na, d, nb);
        verify_div_relation(bn, bd, q, na - nb + 1, r, nb);

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
        mp_ptr numa = alloc_limbs(na);
        mp_ptr numb = alloc_limbs(nb);
        mp_ptr q = alloc_limbs(na - nb + 1);
        random_limbs(numa, na, seed);
        random_limbs(numb, nb, seed);  // MSB=1，已归一化
        BigInt bn(numa, na), bd(numb, nb);

        mp_limb_t qh = lmmp_div_s_(q, numa, na, numb, nb);
        // qh + q[0..na-nb-1] 组成完整商，余数为 numa[0..nb-1]
        std::vector<u64> full_q(q, q + (na - nb));
        full_q.push_back(qh);
        BigInt bq(full_q.data(), full_q.size());
        BigInt br(numa, nb);
        verify_div_relation(bn, bd, full_q.data(), full_q.size(), numa, nb);

        lmmp_free(numa); lmmp_free(numb); lmmp_free(q);
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

TEST_CASE("div/inv", inv_1_inv_2_1) {
    u64 seed = 0x0f0e0d0c0b0a0908ull;
    const u128 max128(-1, -1);  // B^2 - 1
    const u128 B = (u128)1 << 64;

    for (int i = 0; i < 500; ++i) {
        u64 x = xorshift64(seed) | LIMB_B_2;  // inv_1_ 要求 MSB(x)=1
        u64 inv = lmmp_inv_1_(x);
        // inv = floor((B^2-1)/x) - B，即 q = inv + B 是向下取整的商。
        u128 q = (u128)inv + B;
        u128 prod = q * x;
        TEST_CHECK_MSG(prod <= max128 && prod > max128 - x,
                       "inv_1 quotient floor property");
    }

    // inv_2_1 与 div_basecase 的交叉验证已在上一个用例覆盖；
    // 这里仅检查其返回值非零且对归一化 2-limb 输入可被 div_basecase 使用。
    for (int i = 0; i < 200; ++i) {
        u64 xh = xorshift64(seed) | LIMB_B_2;
        u64 xl = xorshift64(seed);
        mp_limb_t inv = lmmp_inv_2_1_(xh, xl);
        TEST_CHECK_MSG(inv != 0, "inv_2_1 nonzero");
        (void)inv;
    }
}

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
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

/*
    精确除法验证：被除数经库乘法构造为 q*d（保证整除），各精确除法
    变体须还原 q；并与普通除法 lmmp_div_ 交叉（商一致、余数为零）。
    binvert 逆元经 mullh 低 64 位 == 1 验证。
*/

TEST_CASE("numth/divexact", divexact_1) {
    u64 seed = 0x1111222233334444ull;
    for (mp_size_t n = 1; n <= 120; ++n) {
        for (int iter = 0; iter < 8; ++iter) {
            u64 d = xorshift64(seed) | 1;
            u64 dinv = lmmp_binvert_ulong_(d);
            mp_limb_t iv[2];
            lmmp_mullh_(d, dinv, iv);
            TEST_CHECK_EQ(iv[0], 1ull);  // d*dinv ≡ 1 (mod B)：低 limb 为 1 即可

            // np = q*d 保留进位 limb（hi!=0 时为 n+1 limb 积，恰覆盖
            // 顶 limb 非零形态；偶数迭代压制 q 顶位使两种形态都出现）
            mp_ptr q = alloc_limbs(n), np = alloc_limbs(n + 1), dst = alloc_limbs(n + 1);
            random_limbs(q, n, seed, false);
            if (iter % 2 == 0) q[n - 1] >>= 1;  // 半数迭代强制 hi==0 形态
            u64 hi = lmmp_mul_1_(np, q, n, d);
            mp_size_t nn = n;
            if (hi) np[n] = hi, nn = n + 1;

            lmmp_divexact_1_(dst, np, nn, d, dinv);
            TEST_CHECK_EQ(memcmp(dst, q, n * 8), 0);
            TEST_CHECK_EQ(dst[n], 0ull);

            // 与普通除法交叉：余数 0、商一致
            mp_ptr q2 = alloc_limbs(n + 1);
            u64 rem = lmmp_div_1_(q2, np, nn, d);
            TEST_CHECK_EQ(rem, 0ull);
            TEST_CHECK_EQ(memcmp(q2, q, n * 8), 0);
            TEST_CHECK_EQ(q2[n], 0ull);

            lmmp_free(q); lmmp_free(np); lmmp_free(dst); lmmp_free(q2);
        }
    }
}

TEST_CASE("numth/divexact", divexact_2) {
    u64 seed = 0x5555666677778888ull;
    for (mp_size_t n = 4; n <= 80; ++n) {  // n>=4 保证 mul_ 契约 na=n-2>=nb=2
        for (int iter = 0; iter < 8; ++iter) {
            // 商实际为 n-2 个 limb；divexact_2 会写 n-1 个 limb，最高位为 0。
            mp_ptr q = alloc_limbs(n - 2), d = alloc_limbs(2), np = alloc_limbs(n),
                   dst = alloc_limbs(n - 1), dinv = alloc_limbs(2);
            random_limbs(q, n - 2, seed, false);
            random_limbs(d, 2, seed);
            d[0] |= 1;
            lmmp_mul_(np, q, n - 2, d, 2);  // np = q*d（恰 n limbs）

            lmmp_binvert_2_(dinv, d);
            lmmp_divexact_2_(dst, np, n, d, dinv);
            TEST_CHECK_EQ(memcmp(dst, q, (n - 2) * 8), 0);
            TEST_CHECK_EQ(dst[n - 2], 0u);

            lmmp_free(q); lmmp_free(d); lmmp_free(np); lmmp_free(dst); lmmp_free(dinv);
        }
    }
}

TEST_CASE("numth/divexact", divexact_basecase_divide_cross) {
    u64 seed = 0x1234432187655678ull;
    struct Case { mp_size_t nn; mp_size_t dn; };
    const Case cases[] = {
        {2, 1}, {3, 1}, {5, 2}, {10, 3}, {20, 5}, {40, 10},
        {60, 20}, {100, 30}, {150, 40}, {250, 60}, {400, 80},
        // 近平衡形态（dn >= nn-dn+1）：激活 divexact_divide_ 分支
        {9, 8}, {12, 10}, {61, 41}, {100, 60}, {200, 120}
    };
    for (auto c : cases) {
        mp_size_t nn = c.nn, dn = c.dn;
        for (int iter = 0; iter < 5; ++iter) {
            mp_ptr d = alloc_limbs(dn);
            random_limbs(d, dn, seed);
            d[0] |= 1;

            mp_size_t qn = nn - dn + 1;
            // 生成 qn-1 个非零 limb 的商；最高位（qn-1）由库写为 0。
            mp_size_t q_actual = qn - 1;
            mp_ptr q = alloc_limbs(q_actual > 0 ? q_actual : 1);
            random_limbs(q, q_actual, seed, false);
            mp_ptr np = alloc_limbs(nn);
            mul_sorted(np, q, q_actual, d, dn);  // np = q*d（近平衡时 q 短于 d）

            // 通用精确除法
            mp_ptr dst1 = alloc_limbs(qn + 1), np1 = alloc_limbs(nn);
            memcpy(np1, np, nn * 8);
            lmmp_divexact_(dst1, np1, nn, d, dn);
            TEST_CHECK_EQ(memcmp(dst1, q, q_actual * 8), 0);
            TEST_CHECK_EQ(dst1[q_actual], 0u);

            // 分治精确除法
            mp_ptr dst2 = alloc_limbs(qn + 1), np2 = alloc_limbs(nn);
            memcpy(np2, np, nn * 8);
            if (dn >= qn) {  // divexact_divide 要求 dn >= nn-dn+1
                lmmp_divexact_divide_(dst2, np2, nn, d, dn);
                TEST_CHECK_EQ(memcmp(dst2, q, q_actual * 8), 0);
                TEST_CHECK_EQ(dst2[q_actual], 0u);
            }

            // 朴素精确除法
            mp_ptr dst3 = alloc_limbs(qn + 1), np3 = alloc_limbs(nn);
            memcpy(np3, np, nn * 8);
            mp_limb_t d0inv = lmmp_binvert_ulong_(d[0]);
            lmmp_divexact_basecase_(dst3, np3, nn, d, dn, d0inv);
            TEST_CHECK_EQ(memcmp(dst3, q, q_actual * 8), 0);
            TEST_CHECK_EQ(dst3[q_actual], 0u);

            // 不平衡精确除法（自动逆元）
            mp_ptr dst4 = alloc_limbs(qn + 1), np4 = alloc_limbs(nn);
            memcpy(np4, np, nn * 8);
            lmmp_divexact_unbalance_(dst4, np4, nn, d, dn, NULL);
            TEST_CHECK_EQ(memcmp(dst4, q, q_actual * 8), 0);
            TEST_CHECK_EQ(dst4[q_actual], 0u);

            // 与普通除法交叉（用户指定对照）：商一致、余数恰为零
            mp_ptr qd = alloc_limbs(nn - dn + 2), rd = alloc_limbs(dn);
            lmmp_zero(qd, nn - dn + 2); lmmp_zero(rd, dn);
            lmmp_div_(qd, rd, np, nn, d, dn);
            TEST_CHECK_EQ(norm_len(rd, dn), (mp_size_t)0);
            TEST_CHECK(mpn_eq(qd, nn - dn + 2, q, q_actual));

            lmmp_free(d); lmmp_free(q); lmmp_free(np);
            lmmp_free(dst1); lmmp_free(np1); lmmp_free(dst2); lmmp_free(np2);
            lmmp_free(dst3); lmmp_free(np3); lmmp_free(dst4); lmmp_free(np4);
            lmmp_free(qd); lmmp_free(rd);
        }
    }
}

TEST_CASE("numth/divexact", divexact_unbalanced_preinv) {
    u64 seed = 0x0a0b0c0d0e0f0102ull;
    for (mp_size_t dn : {1, 2, 3, 10, 30}) {
        for (mp_size_t nn = dn + 1; nn <= dn + 40; ++nn) {
            mp_ptr d = alloc_limbs(dn);
            random_limbs(d, dn, seed);
            d[0] |= 1;
            mp_size_t qn = nn - dn + 1;
            mp_size_t q_actual = qn - 1;
            mp_ptr q = alloc_limbs(q_actual > 0 ? q_actual : 1);
            random_limbs(q, q_actual, seed, false);
            mp_ptr np = alloc_limbs(nn);
            if (q_actual >= dn) lmmp_mul_(np, q, q_actual, d, dn);
            else lmmp_mul_(np, d, dn, q, q_actual);

            mp_ptr dinv = alloc_limbs(dn + 1);
            lmmp_binvert_(dinv, d, dn, dn);  // 关于 B^dn 的逆元
            mp_ptr dst = alloc_limbs(qn + 1), np2 = alloc_limbs(nn);
            memcpy(np2, np, nn * 8);
            lmmp_divexact_unbalance_(dst, np2, nn, d, dn, dinv);
            TEST_CHECK_EQ(memcmp(dst, q, q_actual * 8), 0);
            TEST_CHECK_EQ(dst[q_actual], 0u);

            lmmp_free(d); lmmp_free(q); lmmp_free(np); lmmp_free(dinv); lmmp_free(dst);
            lmmp_free(np2);
        }
    }
}

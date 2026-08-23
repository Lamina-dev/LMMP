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

}  // namespace

TEST_CASE("numth/divexact", divexact_1) {
    u64 seed = 0x1111222233334444ull;
    for (mp_size_t n = 1; n <= 120; ++n) {
        for (int iter = 0; iter < 8; ++iter) {
            u64 d = xorshift64(seed) | 1;
            u64 dinv = lmmp_binvert_ulong_(d);
            TEST_CHECK_MSG((u64)((u128)d * dinv) == 1, "binvert_ulong inverse");

            mp_ptr q = alloc_limbs(n);
            mp_ptr np = alloc_limbs(n);
            mp_ptr dst = alloc_limbs(n);
            random_limbs(q, n, seed, false);
            // np = q * d（精确整除），使用 BigInt 朴素乘法
            BigInt bq(q, n);
            BigInt bd(d);
            BigInt np_val = BigInt::mul_school(bq, bd);
            if (np_val.d.size() > n) { lmmp_free(q); lmmp_free(np); lmmp_free(dst); continue; }
            to_limbs(np_val, np, n);

            lmmp_divexact_1_(dst, np, n, d, dinv);
            TEST_CHECK_MSG(from_limbs(dst, n) == bq, "divexact_1 quotient");

            lmmp_free(q); lmmp_free(np); lmmp_free(dst);
        }
    }
}

TEST_CASE("numth/divexact", divexact_2) {
    u64 seed = 0x5555666677778888ull;
    for (mp_size_t n = 3; n <= 80; ++n) {
        for (int iter = 0; iter < 8; ++iter) {
            // 商实际为 n-2 个 limb；divexact_2 会写 n-1 个 limb，最高位为 0。
            mp_ptr q = alloc_limbs(n - 2);
            mp_ptr d = alloc_limbs(2);
            mp_ptr np = alloc_limbs(n);
            mp_ptr dst = alloc_limbs(n - 1);
            mp_ptr dinv = alloc_limbs(2);
            random_limbs(q, n - 2, seed, false);
            random_limbs(d, 2, seed);
            d[0] |= 1;
            BigInt bq(q, n - 2), bd(d, 2);
            BigInt np_val = BigInt::mul_school(bq, bd);
            if (np_val.d.size() > (size_t)n) { lmmp_free(q); lmmp_free(d); lmmp_free(np); lmmp_free(dst); lmmp_free(dinv); continue; }
            to_limbs(np_val, np, n);

            lmmp_binvert_2_(dinv, d);
            lmmp_divexact_2_(dst, np, n, d, dinv);
            TEST_CHECK_MSG(from_limbs(dst, n - 2) == bq, "divexact_2 quotient");
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
        {60, 20}, {100, 30}, {150, 40}, {250, 60}, {400, 80}
    };
    for (auto c : cases) {
        mp_size_t nn = c.nn, dn = c.dn;
        for (int iter = 0; iter < 5; ++iter) {
            mp_ptr d = alloc_limbs(dn);
            random_limbs(d, dn, seed);
            d[0] |= 1;
            BigInt bd(d, dn);

            mp_size_t qn = nn - dn + 1;
            // 生成 qn-1 个非零 limb 的商；最高位（qn-1）由库写为 0。
            mp_size_t q_actual = qn - 1;
            mp_ptr q = alloc_limbs(q_actual > 0 ? q_actual : 1);
            random_limbs(q, q_actual, seed, false);
            BigInt bq(q, q_actual);
            BigInt np_val = BigInt::mul_school(bq, bd);
            mp_size_t np_size = (mp_size_t)np_val.d.size();
            if (np_size > nn) { lmmp_free(d); lmmp_free(q); continue; }
            mp_ptr np = alloc_limbs(nn);
            to_limbs(np_val, np, nn);

            // 通用精确除法
            mp_ptr dst1 = alloc_limbs(qn + 1);
            mp_ptr np1 = alloc_limbs(nn);
            std::memcpy(np1, np, nn * 8);
            lmmp_divexact_(dst1, np1, nn, d, dn);
            TEST_CHECK_MSG(from_limbs(dst1, q_actual) == bq, "divexact quotient");
            TEST_CHECK_EQ(dst1[q_actual], 0u);

            // 分治精确除法
            mp_ptr dst2 = alloc_limbs(qn + 1);
            mp_ptr np2 = alloc_limbs(nn);
            std::memcpy(np2, np, nn * 8);
            if (dn >= qn) {  // divexact_divide 要求 dn >= nn-dn+1
                lmmp_divexact_divide_(dst2, np2, nn, d, dn);
                TEST_CHECK_MSG(from_limbs(dst2, q_actual) == bq, "divexact_divide quotient");
                TEST_CHECK_EQ(dst2[q_actual], 0u);
            }

            // 朴素精确除法
            mp_ptr dst3 = alloc_limbs(qn + 1);
            mp_ptr np3 = alloc_limbs(nn);
            std::memcpy(np3, np, nn * 8);
            mp_limb_t d0inv = lmmp_binvert_ulong_(d[0]);
            lmmp_divexact_basecase_(dst3, np3, nn, d, dn, d0inv);
            TEST_CHECK_MSG(from_limbs(dst3, q_actual) == bq, "divexact_basecase quotient");
            TEST_CHECK_EQ(dst3[q_actual], 0u);

            // 不平衡精确除法（自动逆元）
            mp_ptr dst4 = alloc_limbs(qn + 1);
            mp_ptr np4 = alloc_limbs(nn);
            std::memcpy(np4, np, nn * 8);
            lmmp_divexact_unbalanced_(dst4, np4, nn, d, dn, NULL);
            TEST_CHECK_MSG(from_limbs(dst4, q_actual) == bq, "divexact_unbalanced quotient");
            TEST_CHECK_EQ(dst4[q_actual], 0u);

            lmmp_free(d); lmmp_free(q); lmmp_free(np);
            lmmp_free(dst1); lmmp_free(np1); lmmp_free(dst2); lmmp_free(np2);
            lmmp_free(dst3); lmmp_free(np3); lmmp_free(dst4); lmmp_free(np4);
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
            BigInt bd(d, dn);
            mp_size_t qn = nn - dn + 1;
            mp_size_t q_actual = qn - 1;
            mp_ptr q = alloc_limbs(q_actual > 0 ? q_actual : 1);
            random_limbs(q, q_actual, seed, false);
            BigInt bq(q, q_actual);
            BigInt np_val = BigInt::mul_school(bq, bd);
            if (np_val.d.size() > (size_t)nn) { lmmp_free(d); lmmp_free(q); continue; }
            mp_ptr np = alloc_limbs(nn);
            to_limbs(np_val, np, nn);

            mp_ptr dinv = alloc_limbs(dn + 1);
            lmmp_binvert_(dinv, d, dn, dn);  // 关于 B^dn 的逆元
            mp_ptr dst = alloc_limbs(qn + 1);
            mp_ptr np2 = alloc_limbs(nn);
            std::memcpy(np2, np, nn * 8);
            lmmp_divexact_unbalanced_(dst, np2, nn, d, dn, dinv);
            TEST_CHECK_MSG(from_limbs(dst, q_actual) == bq, "divexact_unbalanced preinv quotient");
            TEST_CHECK_EQ(dst[q_actual], 0u);

            lmmp_free(d); lmmp_free(q); lmmp_free(np); lmmp_free(dinv); lmmp_free(dst); lmmp_free(np2);
        }
    }
}

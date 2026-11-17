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

TEST_CASE("numth/combin", nPr_nCr_degenerate_boundaries) {
    const ulong npr_ns[] = {0ull, 1ull, 2ull, 3ull, 5ull, 10ull, 12ull};
    for (size_t n_index = 0; n_index < ARRAY_SIZE(npr_ns); ++n_index) {
        ulong n = npr_ns[n_index];
        const ulong npr_rs[] = {0ull, 1ull, n};
        for (size_t r_index = 0; r_index < ARRAY_SIZE(npr_rs); ++r_index) {
            ulong r = npr_rs[r_index];
            if (r > n) continue;
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nPr_size_(n, r, &bits);
            mp_ptr dst = alloc_limbs(need + 2);
            lmmp_zero(dst, need + 2);
            mp_size_t rn = lmmp_nPr_(dst, bits, need, n, r);  // 实际长度
            if (r == 0) {
                TEST_CHECK_EQ(dst[0], 1ull);
                TEST_CHECK_EQ(rn, (mp_size_t)1);
            } else if (r == 1) {
                TEST_CHECK_EQ(dst[0], (u64)n);  // n <= 12 单 limb
                TEST_CHECK_EQ(rn, (mp_size_t)1);
            } else {
                // nPr(n, n) == n!：库 factorial 交叉（长度一律取返回值，
                // size_ 含冗余、缓冲高位为未初始化内存）
                mp_bitcnt_t fb = 0;
                mp_size_t fn = lmmp_factorial_size_((uint)n, &fb);
                mp_ptr f = alloc_limbs(fn + 2);
                mp_size_t fl = lmmp_factorial_(f, fb, fn, (uint)n);
                TEST_CHECK(mpn_eq(dst, need + 2, f, fl));  // dst 已零化，容量域比较安全
                TEST_CHECK_EQ(rn, fl);  // 同值规范化长度必等，防高位零掩盖
                lmmp_free(f);
            }
            lmmp_free(dst);
        }
    }

    const uint ncr_ns[] = {0u, 1u, 2u, 5u, 10u, 20u};
    for (size_t n_index = 0; n_index < ARRAY_SIZE(ncr_ns); ++n_index) {
        uint n = ncr_ns[n_index];
        uint half = n / 2;
        const uint ncr_rs[] = {0u, 1u, half};
        for (size_t r_index = 0; r_index < ARRAY_SIZE(ncr_rs); ++r_index) {
            uint r = ncr_rs[r_index];
            if (r > half) continue; /* nCr 要求 r <= n/2 */
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nCr_size_(n, r, &bits);
            mp_ptr dst = alloc_limbs(need + 2);
            lmmp_zero(dst, need + 2);
            mp_size_t dn = lmmp_nCr_(dst, bits, need, n, r);  // 实际长度
            if (r == 0) {
                TEST_CHECK_EQ(dst[0], 1ull);
            } else if (r == 1) {
                TEST_CHECK_EQ(dst[0], (u64)n);
            } else {
                // C(n,k)*k!*(n-k)! == n!（库乘 + factorial 交叉；长度取
                // 返回值，乘法乒乓双缓冲保持 sep(dst,a,b)）
                mp_bitcnt_t b1 = 0, b2 = 0, b3 = 0;
                mp_size_t n1 = lmmp_factorial_size_(r, &b1);
                mp_size_t n2 = lmmp_factorial_size_(n - r, &b2);
                mp_size_t n3 = lmmp_factorial_size_(n, &b3);
                mp_ptr f1 = alloc_limbs(n1 + 2), f2 = alloc_limbs(n2 + 2), f3 = alloc_limbs(n3 + 2),
                       prod = alloc_limbs(need + n3 + 4), prod2 = alloc_limbs(need + n3 + 4);
                mp_size_t l1 = lmmp_factorial_(f1, b1, n1, r);
                mp_size_t l2 = lmmp_factorial_(f2, b2, n2, n - r);
                mp_size_t l3 = lmmp_factorial_(f3, b3, n3, n);
                lmmp_mul_(prod, dst, dn, f1, l1);
                mp_size_t pn = dn + l1;
                lmmp_mul_(prod2, prod, pn, f2, l2);
                mp_size_t p2n = pn + l2;
                TEST_CHECK(mpn_eq(prod2, p2n, f3, l3));
                lmmp_free(f1); lmmp_free(f2); lmmp_free(f3);
                lmmp_free(prod); lmmp_free(prod2);
            }
            lmmp_free(dst);
        }
    }

    {
        ulong n = 0xffff;
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_nPr_size_(n, 1, &bits);
        mp_ptr dst = alloc_limbs(need + 2);
        lmmp_zero(dst, need + 2);
        mp_size_t rn = lmmp_nPr_(dst, bits, need, n, 1);
        TEST_CHECK(mpn_eq(dst, need + 2, &n, 1));  // nPr(n,1) == n
        TEST_CHECK_EQ(rn, (mp_size_t)1);
        lmmp_free(dst);
    }
}

void test_combin_boundaries(void) {
    TEST_RUN("numth/combin", nPr_nCr_degenerate_boundaries);
}

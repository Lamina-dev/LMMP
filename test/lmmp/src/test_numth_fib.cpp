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

using namespace lmmp_test_utils;

/*
    斐波那契验证（库内交叉 + 恒等式 + 写死常数）：
      1. 写死锚点：F(10)=55、F(50)=12586269025、F(93)=12200160415121876738
      2. 链式递推：fibonacci2_(n) 的 F[n-1]/F[n] 与 fibonacci2_(n+1) 的
         F[n]/F[n+1] 逐档衔接，且 F[n+1] == F[n] + F[n-1]（库内加法）
      3. 大规模抽查：递推链 + Cassini 恒等式（全库内，见下）
*/

TEST_CASE("numth/fib", small_exhaustive) {
    // 写死常数锚点（值均 < 2^64）
    struct { ulong n; u64 v; } known[] = {
        {10, 55ull}, {50, 12586269025ull},
        {93, 12200160415121876738ull},  // FIB_TABLE_LIMIT 档（最后单 limb 表项）
    };
    for (auto& k : known) {
        mp_ptr d = alloc_limbs(2);
        lmmp_zero(d, 2);
        lmmp_fibonacci_(d, 2, k.n);
        TEST_CHECK_EQ(d[0], k.v);
        lmmp_free(d);
    }

    // 链式递推：F[n+1] == F[n] + F[n-1]，两档 fibonacci2_ 输出衔接
    const ulong NLIM = 500;
    for (ulong n = 1; n <= NLIM; n += 7) {
        mp_size_t rn = lmmp_fibonacci_size_(n + 1) + 1;
        mp_ptr fa = alloc_limbs((size_t)rn), fb = alloc_limbs((size_t)rn);
        mp_ptr fc = alloc_limbs((size_t)rn), fd = alloc_limbs((size_t)rn);
        mp_ptr s = alloc_limbs((size_t)rn + 1);
        lmmp_zero(fa, rn); lmmp_zero(fb, rn); lmmp_zero(fc, rn); lmmp_zero(fd, rn);

        lmmp_fibonacci2_(fb, fa, rn, n);      // fa = F[n-1], fb = F[n]
        lmmp_fibonacci2_(fd, fc, rn, n + 1);  // fc = F[n],   fd = F[n+1]
        TEST_CHECK(mpn_eq(fb, rn, fc, rn));   // 两档衔接：同 n 输出一致
        lmmp_zero(s, rn + 1);
        lmmp_add_(s, fb, rn, fa, rn);
        TEST_CHECK(mpn_eq(s, rn + 1, fd, rn));  // F[n]+F[n-1] == F[n+1]

        lmmp_free(fa); lmmp_free(fb); lmmp_free(fc); lmmp_free(fd); lmmp_free(s);
    }
}

/* 大规模抽查：递推链 F[n]=F[n-1]+F[n-2] 与 Cassini 恒等式 F[n-1]*F[n+1]-F[n]^2=(-1)^n，
   均以库内加/乘法验证，覆盖奇偶、2 的幂、fibonacci_ 合并路径中 2F[k] 溢出的位组合 */

TEST_CASE("numth/fib", large_recurrence_and_cassini) {
    for (ulong n : {94ul, 95ul, 187ul, 371ul, 373ul, 925ul, 1000ul, 4181ul, 65536ul, 65537ul,
                    100001ul, 131072ul, 150000ul}) {
        mp_size_t rn = lmmp_fibonacci_size_(n + 1);
        mp_ptr a = alloc_limbs((size_t)rn);        // F[n-1]
        mp_ptr b = alloc_limbs((size_t)rn);        // F[n]
        mp_ptr c = alloc_limbs((size_t)rn);        // F[n+1]
        mp_ptr t = alloc_limbs((size_t)(2 * rn));  // fibonacci2_ 副输出 / F[n]^2
        mp_ptr s = alloc_limbs((size_t)(2 * rn));

        lmmp_zero(a, rn); lmmp_zero(b, rn); lmmp_zero(c, rn);
        mp_size_t lb = lmmp_fibonacci2_(b, a, rn, n);
        lmmp_zero(b + lb, rn - lb); lmmp_zero(a + lb, rn - lb);
        mp_size_t lc = lmmp_fibonacci2_(c, t, rn, n + 1);
        lmmp_zero(c + lc, rn - lc);

        // 递推链：F[n+1] == F[n] + F[n-1]
        lmmp_zero(s, 2 * rn);
        mp_limb_t carry = lmmp_add_(s, b, rn, a, rn);
        TEST_CHECK_MSG(carry == 0, "recurrence no carry");
        TEST_CHECK_MSG(norm_len(s, 2 * rn) == lc, "recurrence size");
        for (mp_size_t i = 0; i < lc; ++i) {
            if (s[i] != c[i]) {
                TEST_CHECK_MSG(false, "recurrence value");
                break;
            }
        }

        // fibonacci_ 与 fibonacci2_ 一致
        mp_ptr d = alloc_limbs((size_t)lmmp_fibonacci_size_(n));
        mp_size_t ld = lmmp_fibonacci_(d, lmmp_fibonacci_size_(n), n);
        TEST_CHECK_MSG(ld == lb, "consistency size");
        bool same = true;
        for (mp_size_t i = 0; i < ld; ++i) same = same && (d[i] == b[i]);
        TEST_CHECK_MSG(same, "consistency value");

        // Cassini：F[n-1]*F[n+1] - F[n]^2 == (-1)^n
        lmmp_mul_(s, a, rn, c, rn);
        lmmp_sqr_(t, b, rn);
        mp_limb_t borrow = lmmp_sub_(s, s, 2 * rn, t, 2 * rn);
        if (n % 2 == 0) {
            TEST_CHECK_MSG(borrow == 0 && s[0] == 1 && norm_len(s, 2 * rn) == 1, "Cassini +1");
        } else {
            // -1 即 2*rn 个全 F limb
            bool allf = true;
            for (mp_size_t i = 0; i < 2 * rn; ++i) allf = allf && (s[i] == LIMB_MAX);
            TEST_CHECK_MSG(borrow == 1 && allf, "Cassini -1");
        }

        lmmp_free(a); lmmp_free(b); lmmp_free(c); lmmp_free(t); lmmp_free(s); lmmp_free(d);
    }
}

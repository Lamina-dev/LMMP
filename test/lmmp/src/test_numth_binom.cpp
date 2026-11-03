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

/* 排列组合族验证（无 BigInt oracle；阶乘族见 test_numth_combin.cpp）：
   1. 递推链：nPr == 逐项乘积链（库 mul_1）
   2. 恒等式交叉：nPr*(n-r)! == n!、帕斯卡 C(n,k) == C(n-1,k-1)+C(n,k-1)、
      C(n,k)*k!*(n-k)! == n!（库 div/mul/add）
   3. 写死锚点：C(10,5)=252、C(52,5)=2598960、multinomial(1,2,3)=60
   4. arith_seqprod：链乘参考（库 mul_1）
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

using namespace lmmp_test_utils;

TEST_CASE("numth/number", nPr_nCr_multinomial_arith) {
    // nPr == 乘积链（库 mul_1）+ 恒等式 nPr*(n-r)! == n!（库乘重建）
    const size_t CAP = 2048;
    mp_ptr e = alloc_limbs(CAP), f1 = alloc_limbs(CAP), f2 = alloc_limbs(CAP),
           prod = alloc_limbs(CAP);
    for (ulong n : {0, 1, 5, 10, 20, 50, 100}) {
        for (ulong r = 0; r <= n; r += (n > 20 ? 20 : 1)) {
            // 链参考
            e[0] = 1;
            mp_size_t elen = 1;
            for (u32 i = (u32)(n - r + 1); i <= n; ++i) elen = chain_mul_small(e, elen, i, CAP);

            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nPr_size_(n, r, &bits);
            mp_ptr dst = alloc_limbs(need + 2);
            lmmp_zero(dst, need + 2);
            lmmp_nPr_(dst, bits, need, n, r);
            TEST_CHECK(mpn_eq(dst, need + 2, e, elen));

            // 恒等式：nPr * (n-r)! == n!（r>=2 时交叉）
            if (r >= 2 && n - r >= 1) {
                mp_bitcnt_t b1 = 0, b2 = 0;
                mp_size_t need1 = lmmp_factorial_size_((u32)n, &b1);
                mp_size_t need2 = lmmp_factorial_size_((u32)(n - r), &b2);
                lmmp_zero(f1, need1 + 2); lmmp_zero(f2, need2 + 2);
                lmmp_factorial_(f1, b1, need1, (u32)n);
                lmmp_factorial_(f2, b2, need2, (u32)(n - r));
                mp_size_t l1 = norm_len(f1, need1 + 2), l2 = norm_len(f2, need2 + 2);
                mp_size_t ldst = norm_len(dst, need + 2);
                mul_sorted(prod, dst, ldst, f2, l2);
                mp_size_t lp = norm_len(prod, ldst + l2);
                TEST_CHECK(mpn_eq(prod, lp, f1, l1));  // nPr * (n-r)! == n!
            }
            lmmp_free(dst);
        }
    }

    // nCr：帕斯卡恒等式 + C(n,k)*k!*(n-k)! == n! + 写死锚点
    {
        struct { u32 n; u32 r; u64 v; } known[] = {{10, 5, 252ull}, {52, 5, 2598960ull}};
        for (auto& k : known) {
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nCr_size_(k.n, k.r, &bits);
            mp_ptr dst = alloc_limbs(need + 2);
            lmmp_zero(dst, need + 2);
            lmmp_nCr_(dst, bits, need, k.n, k.r);
            TEST_CHECK_EQ(dst[0], k.v);
            TEST_CHECK_EQ(norm_len(dst + 1, need + 1), 0);
            lmmp_free(dst);
        }
    }
    mp_ptr cn = alloc_limbs(CAP), cn1 = alloc_limbs(CAP), cn2 = alloc_limbs(CAP);
    for (u32 n : {2, 5, 10, 20, 50}) {
        for (u32 r = 1; r <= n / 2; ++r) {
            mp_bitcnt_t b0 = 0, b1 = 0, b2 = 0;
            mp_size_t nd0 = lmmp_nCr_size_(n, r, &b0);
            mp_size_t nd1 = lmmp_nCr_size_(n - 1, r - 1, &b1);      // 契约 r<=n/2 → r-1<=(n-1)/2 ✓
            mp_size_t nd2 = n - r > r ? lmmp_nCr_size_(n - 1, r, &b2) : 0;  // 需 r <= (n-1)/2
            lmmp_zero(cn, nd0 + 2);
            lmmp_nCr_(cn, b0, nd0, n, r);
            if (nd2) {
                // 帕斯卡：C(n,r) == C(n-1,r-1) + C(n-1,r)
                lmmp_zero(cn1, nd1 + 2); lmmp_zero(cn2, nd2 + 2);
                lmmp_nCr_(cn1, b1, nd1, n - 1, r - 1);
                lmmp_nCr_(cn2, b2, nd2, n - 1, r);
                mp_size_t l1 = norm_len(cn1, nd1 + 2), l2 = norm_len(cn2, nd2 + 2);
                // 零垫对齐后加
                mp_ptr pad = alloc_limbs(CAP), sum = alloc_limbs(CAP);
                size_t mx = (size_t)(l1 > l2 ? l1 : l2) + 1;
                lmmp_zero(pad, mx);
                memcpy(pad, cn1, (size_t)l1 * 8);
                lmmp_zero(sum, mx);
                memcpy(sum, cn2, (size_t)l2 * 8);
                lmmp_add_n_(sum, sum, pad, mx);
                TEST_CHECK(mpn_eq(sum, mx, cn, norm_len(cn, nd0 + 2)));
                lmmp_free(pad);
                lmmp_free(sum);
            }

            // 恒等式：C(n,r) * r! * (n-r)! == n!
            if (n >= 2) {
                mp_bitcnt_t bfr = 0, bfnr = 0, bfn = 0;
                mp_size_t nfr = lmmp_factorial_size_(r, &bfr);
                mp_size_t nfnr = lmmp_factorial_size_(n - r, &bfnr);
                mp_size_t nfn = lmmp_factorial_size_(n, &bfn);
                lmmp_zero(f1, nfr + 2); lmmp_zero(f2, nfnr + 2);
                lmmp_factorial_(f1, bfr, nfr, r);
                lmmp_factorial_(f2, bfnr, nfnr, n - r);
                mp_ptr rem = alloc_limbs(nfn + 2);
                lmmp_zero(rem, nfn + 2);
                lmmp_factorial_(rem, bfn, nfn, n);  // n!
                mp_size_t lfr = norm_len(f1, nfr + 2), lfnr = norm_len(f2, nfnr + 2);
                mp_size_t lcn = norm_len(cn, nd0 + 2), lfn = norm_len(rem, nfn + 2);
                mp_ptr quot = alloc_limbs(CAP);
                mul_sorted(prod, cn, lcn, f1, lfr);
                mp_size_t lp = norm_len(prod, lcn + lfr);
                mul_sorted(quot, prod, lp, f2, lfnr);
                lp = norm_len(quot, lp + lfnr);
                TEST_CHECK(mpn_eq(quot, lp, rem, lfn));  // C*r!*(n-r)! == n!
                lmmp_free(quot);
                lmmp_free(rem);
            }
        }
    }
    lmmp_free(cn); lmmp_free(cn1); lmmp_free(cn2);

    // multinomial：写死锚点 6!/(1!2!3!) = 60（恒等式交叉由 nCr 的
    // C*k!*(n-k)!==n! 覆盖，同构）
    {
        u32 r[] = {1, 2, 3};
        u32 n_sum = 6;
        ulong n = 0;
        mp_size_t need = lmmp_multinomial_size_(r, 3, &n);
        TEST_CHECK_EQ(n, (ulong)n_sum);
        mp_ptr dst = alloc_limbs(need + 2);
        lmmp_zero(dst, need + 2);
        lmmp_multinomial_(dst, need, (uint)n, r, 3);
        TEST_CHECK_EQ(dst[0], 60ull);
        TEST_CHECK_EQ(norm_len(dst + 1, need + 1), 0);
        lmmp_free(dst);
    }

    // arith_seqprod: 链乘参考（库 mul_1）
    for (u32 x : {1, 3, 10}) {
        for (u32 m : {2, 5, 10}) {
            for (u32 n : {1, 3, 10}) {
                if ((u64)x + (u64)n * m > 100000) continue;
                e[0] = 1;
                mp_size_t elen = 1;
                for (u32 i = 0; i <= n; ++i) elen = chain_mul_small(e, elen, x + i * m, CAP);
                mp_size_t need = lmmp_arith_seqprod_size_(x, n, m);
                mp_ptr dst = alloc_limbs(need + 2);
                lmmp_zero(dst, need + 2);
                lmmp_arith_seqprod_(dst, need, x, n, m);
                TEST_CHECK(mpn_eq(dst, need + 2, e, elen));
                lmmp_free(dst);
            }
        }
    }
    lmmp_free(e); lmmp_free(f1); lmmp_free(f2); lmmp_free(prod);
}

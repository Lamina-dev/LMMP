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

/* 阶乘族与因子剥离验证（无 BigInt oracle；nPr/nCr 见 test_numth_binom.cpp）：
   1. 递推链：n! == n*(n-1)!、n!! == n*(n-2)!!、H(n) == H(n-1)*n^n、
      S(n) == S(n-1)*n!（乘法全走库 mul_1/mul/factorial_）
   2. 模检查：结果 mod 32 位素数 == 逐项模乘（u64 简单循环）
   3. 写死锚点：10!=3628800、20!=2432902008176640000、9!!=945、
      10!!=3840
   4. remove 剥除恰 k 个因子（库构造）
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

using namespace lmmp_test_utils;

TEST_CASE("numth/fac", factorial) {
    const size_t CAP = 4000;
    mp_ptr expect = alloc_limbs(CAP);
    mp_size_t elen = 0;

    // 写死锚点
    {
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_factorial_size_(10, &bits);
        mp_ptr dst = alloc_limbs(need + 2);
        lmmp_zero(dst, need + 2);
        lmmp_factorial_(dst, bits, need, 10);
        TEST_CHECK_EQ(dst[0], 3628800ull);
        TEST_CHECK_EQ(norm_len(dst + 1, need + 1), (mp_size_t)0);
        lmmp_free(dst);
    }
    {
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_factorial_size_(20, &bits);
        mp_ptr dst = alloc_limbs(need + 2);
        lmmp_zero(dst, need + 2);
        lmmp_factorial_(dst, bits, need, 20);
        TEST_CHECK_EQ(dst[0], 2432902008176640000ull);
        TEST_CHECK_EQ(norm_len(dst + 1, need + 1), (mp_size_t)0);
        lmmp_free(dst);
    }

    // 递推链 + 模检查：n! == n * (n-1)!（链跨全部中间档位）
    u32 built = 0;  // expect == built!
    expect[0] = 1;
    elen = 1;
    for (u32 n : {0, 1, 2, 5, 10, 20, 50, 100, 200, 1000, 10000}) {
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_factorial_size_(n, &bits);
        mp_ptr dst = alloc_limbs(need + 2);
        lmmp_zero(dst, need + 2);
        lmmp_factorial_(dst, bits, need, n);
        mp_size_t rn = norm_len(dst, need + 2);
        if (n == 0) {
            TEST_CHECK_EQ(dst[0], 1ull);
            lmmp_free(dst);
            continue;
        }
        for (u32 i = built + 1; i <= n; ++i) elen = chain_mul_small(expect, elen, i, CAP);
        built = n;
        TEST_CHECK(mpn_eq(dst, rn, expect, elen));

        // 模检查：n! mod p == 逐项模乘（独立 64 位计算）
        for (int pi = 0; pi < MOD_P_CNT; ++pi) {
            u64 p = MOD_P[pi];
            u64 m = 1;
            for (u32 i = 2; i <= n; ++i) m = mulmod32(m, i % p, p);
            TEST_CHECK_EQ(mod32(dst, rn, (u32)p), m);
        }
        lmmp_free(dst);
    }

    // 筛表路径（n > 65535 触发 lmmp_prime_int_table_init_ 的 mod-30 位图）：
    // 递推链过长，改用三素数独立模锚点（素数集错误必致模值不等）
    {
        u32 n = 100003;
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_factorial_size_(n, &bits);
        mp_ptr dst = alloc_limbs(need + 2);
        lmmp_zero(dst, need + 2);
        lmmp_factorial_(dst, bits, need, n);
        mp_size_t rn = norm_len(dst, need + 2);
        for (int pi = 0; pi < MOD_P_CNT; ++pi) {
            u64 p = MOD_P[pi];
            u64 m = 1;
            for (u32 i = 2; i <= n; ++i) m = mulmod32(m, i % p, p);
            TEST_CHECK_EQ(mod32(dst, rn, (u32)p), m);
        }
        lmmp_free(dst);
    }
    lmmp_free(expect);
}

TEST_CASE("numth/fac_extern", double_hyper_super_primefac) {
    // 双阶乘：n!! == n*(n-2)!! 链 + 写死锚点 9!!=945、10!!=3840
    {
        struct { u32 n; u64 v; } known[] = {{0, 1ull}, {1, 1ull}, {9, 945ull}, {10, 3840ull}};
        for (auto& k : known) {
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_2factorial_size_(k.n, &bits);
            mp_ptr dst = alloc_limbs(need + 2);
            lmmp_zero(dst, need + 2);
            lmmp_2factorial_(dst, bits, need, k.n);
            TEST_CHECK_EQ(dst[0], k.v);
            TEST_CHECK_EQ(norm_len(dst + 1, need + 1), (mp_size_t)0);
            lmmp_free(dst);
        }
    }
    const size_t CAP = 512;
    mp_ptr e = alloc_limbs(CAP);
    for (u32 n : {2, 3, 10, 21, 50, 107, 301}) {
        // 链参考：从 1 起按 n 的奇偶逐项乘
        e[0] = 1;
        mp_size_t elen = 1;
        for (u32 i = (n % 2 == 0 ? 2 : 1); i <= n; i += 2)
            elen = chain_mul_small(e, elen, i, CAP);
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_2factorial_size_(n, &bits);
        mp_ptr dst = alloc_limbs(need + 2);
        lmmp_zero(dst, need + 2);
        lmmp_2factorial_(dst, bits, need, n);
        TEST_CHECK(mpn_eq(dst, need + 2, e, elen));
        lmmp_free(dst);
    }
    lmmp_free(e);

    // hyper factorial: H(n) == H(n-1) * n^n（n^n 由库 mul 链构造）
    const size_t CAP2 = 4096;
    mp_ptr h = alloc_limbs(CAP2), t = alloc_limbs(CAP2);
    h[0] = 1;
    mp_size_t hlen = 1;
    int checked_h = 0;
    for (u32 k = 1; k <= 20; ++k) {
        // k^k：k 次乘 k
        t[0] = 1;
        mp_size_t tn = 1;
        for (u32 i = 0; i < k; ++i) tn = chain_mul_small(t, tn, k, CAP2);
        mp_ptr prod = alloc_limbs(CAP2);
        mul_sorted(prod, h, hlen, t, tn);
        hlen = norm_len(prod, hlen + tn);
        memcpy(h, prod, (size_t)hlen * 8);
        lmmp_free(prod);
        if (k == 1 || k == 3 || k == 5 || k == 8 || k == 20) {
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_hyperfac_size_(k, &bits);
            mp_ptr dst = alloc_limbs(need + 2);
            lmmp_zero(dst, need + 2);
            lmmp_hyperfac_(dst, bits, need, k);
            TEST_CHECK(mpn_eq(dst, need + 2, h, hlen));
            lmmp_free(dst);
            ++checked_h;
        }
    }
    TEST_CHECK_EQ(checked_h, 5);
    lmmp_free(h);
    lmmp_free(t);

    // super factorial: S(n) == S(n-1) * n!（n! 用库 factorial_，交叉）
    const size_t CAP3 = 4096;
    mp_ptr s = alloc_limbs(CAP3);
    s[0] = 1;
    mp_size_t slen = 1;
    for (u32 k = 1; k <= 30; ++k) {
        mp_bitcnt_t bits = 0;
        mp_size_t need = lmmp_factorial_size_(k, &bits);
        mp_ptr fac = alloc_limbs(need + 2);
        lmmp_zero(fac, need + 2);
        lmmp_factorial_(fac, bits, need, k);
        mp_size_t flen = norm_len(fac, need + 2);

        mp_ptr prod = alloc_limbs(CAP3);
        mul_sorted(prod, s, slen, fac, flen);
        slen = norm_len(prod, slen + flen);
        memcpy(s, prod, (size_t)slen * 8);

        if (k == 1 || k == 3 || k == 5 || k == 8 || k == 30) {
            need = lmmp_superfac_size_(k, &bits);
            mp_ptr dst = alloc_limbs(need + 2);
            lmmp_zero(dst, need + 2);
            lmmp_superfac_(dst, bits, need, k);
            TEST_CHECK(mpn_eq(dst, need + 2, s, slen));
            lmmp_free(dst);
        }
        lmmp_free(prod);
        lmmp_free(fac);
    }
    lmmp_free(s);

    // prime factorial: 素数判定用 64 位内简单试除，乘积走库链
    const size_t CAP4 = 4096;
    mp_ptr pf = alloc_limbs(CAP4);
    for (u32 n : {2, 3, 10, 20, 50, 700}) {
        pf[0] = 1;
        mp_size_t plen = 1;
        for (u32 p = 2; p <= n; ++p)
            if (is_prime_small(p)) plen = chain_mul_small(pf, plen, p, CAP4);
        mp_size_t need = lmmp_primefac_size_(n);
        mp_ptr dst = alloc_limbs(need + 2);
        lmmp_zero(dst, need + 2);
        lmmp_primefac_(dst, need, n);
        TEST_CHECK(mpn_eq(dst, need + 2, pf, plen));
        lmmp_free(dst);
    }
    lmmp_free(pf);
}

TEST_CASE("numth/remove", remove_factors) {
    // remove：np = dp^k * 11（库 mul_1 链构造），须剥除恰 k 个因子剩 11
    for (u64 dp : {2, 3, 5, 7}) {
        mp_ptr np = alloc_limbs(3), d = alloc_limbs(1);
        d[0] = dp;
        for (int k = 0; k < 4; ++k) {
            np[0] = 1;
            np[1] = 0;
            np[2] = 0;
            mp_size_t nn = 1;
            for (int i = 0; i < k; ++i) nn = chain_mul_small(np, nn, dp, 3);
            nn = chain_mul_small(np, nn, 11, 3);

            mp_size_t cnt = lmmp_remove_(np, &nn, d, 1);
            TEST_CHECK_EQ(cnt, (mp_size_t)k);
            TEST_CHECK_EQ(nn, (mp_size_t)1);
            TEST_CHECK_EQ(np[0], 11ull);
        }
        lmmp_free(np);
        lmmp_free(d);
    }
}

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
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

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
    lmmp_add_n_(r, a, b, n);   // 分离缓冲参考：和
    lmmp_sub_n_(d, a, b, n);   // 分离缓冲参考：差（a >= b）
    // eqsep 原地：dst == numa / dst == numb 各两向
    memcpy(a2, a, n * 8);
    lmmp_add_n_(a2, a2, b, n);
    TEST_CHECK_EQ(memcmp(a2, r, n * 8), 0);
    memcpy(b2, b, n * 8);
    lmmp_add_n_(b2, a, b2, n);
    TEST_CHECK_EQ(memcmp(b2, r, n * 8), 0);
    memcpy(a2, a, n * 8);
    lmmp_sub_n_(a2, a2, b, n);
    TEST_CHECK_EQ(memcmp(a2, d, n * 8), 0);
    memcpy(b2, b, n * 8);
    lmmp_sub_n_(b2, a, b2, n);
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

    for (mp_size_t s : {0, 1, 7, 31, 63}) {
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
    for (mp_size_t n : ns) {
        for (u64 fill : fills) {
            mp_ptr a = alloc_limbs(n), q = alloc_limbs(n + 1), t = alloc_limbs(n + 2);
            for (mp_size_t i = 0; i < n; ++i) a[i] = (i == n - 1) ? fill : (fill >> 1);
            for (u64 x : divisors) {
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

TEST_CASE("numth/combin", nPr_nCr_degenerate_boundaries) {
    const ulong npr_ns[] = {0ull, 1ull, 2ull, 3ull, 5ull, 10ull, 12ull};
    for (ulong n : npr_ns) {
        const ulong npr_rs[] = {0ull, 1ull, n};
        for (ulong r : npr_rs) {
            if (r > n) continue;
            mp_bitcnt_t bits = 0;
            mp_size_t need = lmmp_nPr_size_(n, r, &bits);
            mp_ptr dst = alloc_limbs(need + 2);
            lmmp_zero(dst, need + 2);
            lmmp_nPr_(dst, bits, need, n, r);
            if (r == 0) {
                TEST_CHECK_EQ(dst[0], 1ull);
            } else if (r == 1) {
                TEST_CHECK_EQ(dst[0], (u64)n);  // n <= 12 单 limb
            } else {
                // nPr(n, n) == n!：库 factorial 交叉（长度一律取返回值，
                // size_ 含冗余、缓冲高位为未初始化内存）
                mp_bitcnt_t fb = 0;
                mp_size_t fn = lmmp_factorial_size_((uint)n, &fb);
                mp_ptr f = alloc_limbs(fn + 2);
                mp_size_t fl = lmmp_factorial_(f, fb, fn, (uint)n);
                TEST_CHECK(mpn_eq(dst, need + 2, f, fl));  // dst 已零化，容量域比较安全
                lmmp_free(f);
            }
            lmmp_free(dst);
        }
    }

    const uint ncr_ns[] = {0u, 1u, 2u, 5u, 10u, 20u};
    for (uint n : ncr_ns) {
        uint half = n / 2;
        const uint ncr_rs[] = {0u, 1u, half};
        for (uint r : ncr_rs) {
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
        lmmp_nPr_(dst, bits, need, n, 1);
        TEST_CHECK(mpn_eq(dst, need + 2, &n, 1));  // nPr(n,1) == n
        lmmp_free(dst);
    }
}

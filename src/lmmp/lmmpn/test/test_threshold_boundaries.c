/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
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

/*
    算法阈值边界回归（无 BigInt oracle；模式同 test_mul_toom68）：
    阈值 ±1 两侧的多路径互比 + basecase 精确对拍 + 模检查。

      - mul_n_：toom22/33/44 阈值 ±1 经完整分派，与 mul_basecase_
        逐 limb 精确一致 + 乘积模检查
      - mullo_：MULLO_DC_THRESHOLD ±1 低 n limb 与 lmmp_mul_ 全积一致
      - div_：DIV_DIVIDE_THRESHOLD ±1 q*d+r 重建 + 余数界
      - divexact_：DIVEXACT_BASECASE 门 ±1 × 小/大 nn 形状（NN 门已退役，
        dn<BC 对任意 nn 走 basecase），n = q*d 构造恢复商
      - pow_：POW_1_EXP_THRESHOLD ±1 与 pow_1_ 单 limb 域精确互比
        + 模幂参考
*/

#include "mparam.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

// 32 位素数模幂参考
static u64 modpow32(u64 base, u64 exp, u32 p) {
    u64 r = 1 % p, a = base % p;
    while (exp) {
        if (exp & 1) r = r * a % p;
        a = a * a % p;
        exp >>= 1;
    }
    return r;
}

static void fill_deterministic(mp_ptr p, mp_size_t n, u64 seed) {
    for (mp_size_t i = 0; i < n; ++i) p[i] = xorshift64(&seed);
    if (n > 0) p[n - 1] |= (u64)1 << 63;
}

// mul_n 阈值边界：与 basecase 逐 limb 精确一致 + 乘积模检查
static void check_mul_n(mp_size_t n, u64 seed) {
    mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), d = alloc_limbs(2 * n + 2),
           ref = alloc_limbs(2 * n + 2);
    fill_deterministic(a, n, seed);
    fill_deterministic(b, n, seed ^ 0x9e3779b97f4a7c15ull);

    lmmp_mul_n_(d, a, b, n);
    lmmp_mul_basecase_(ref, a, n, b, n);
    bool ok = memcmp(d, ref, (size_t)(2 * n) * 8) == 0;
    ok = ok && mod_is_product(d, 2 * n, a, n, b, n);
    TEST_CHECK_MSG(ok, "mul_n at threshold boundary");
    lmmp_free(a);
    lmmp_free(b);
    lmmp_free(d);
    lmmp_free(ref);
}

TEST_CASE("threshold/mul", mul_n_toom_boundaries) {
    check_mul_n(MUL_TOOM22_THRESHOLD > 1 ? MUL_TOOM22_THRESHOLD - 1 : 1, 0x1111222233334444ull);
    check_mul_n(MUL_TOOM22_THRESHOLD,     0x2222333344445555ull);
    check_mul_n(MUL_TOOM22_THRESHOLD + 1, 0x3333444455556666ull);

    check_mul_n(MUL_TOOM33_THRESHOLD > 1 ? MUL_TOOM33_THRESHOLD - 1 : 1, 0x4444555566667777ull);
    check_mul_n(MUL_TOOM33_THRESHOLD,     0x5555666677778888ull);
    check_mul_n(MUL_TOOM33_THRESHOLD + 1, 0x6666777788889999ull);

    check_mul_n(MUL_TOOM44_THRESHOLD > 1 ? MUL_TOOM44_THRESHOLD - 1 : 1, 0x777788889999aaabull);
    check_mul_n(MUL_TOOM44_THRESHOLD,     0x88889999aaaabbbbull);
    check_mul_n(MUL_TOOM44_THRESHOLD + 1, 0x9999aaaabbbbccccull);
}

TEST_CASE("threshold/mullo", mullo_dc_boundary) {
    const mp_size_t sizes[] = {MULLO_DC_THRESHOLD > 1 ? MULLO_DC_THRESHOLD - 1 : 1, MULLO_DC_THRESHOLD,
                             MULLO_DC_THRESHOLD + 1};
    u64 seed = 0xabcddcba12344321ull;
    for (size_t n_index = 0; n_index < ARRAY_SIZE(sizes); ++n_index) {
        mp_size_t n = sizes[n_index];
        mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), lo = alloc_limbs(n),
               full = alloc_limbs(2 * n + 2);
        fill_deterministic(a, n, seed);
        fill_deterministic(b, n, seed ^ 0x13579bdf2468ace0ull);

        lmmp_mul_(full, a, n, b, n);
        lmmp_mullo_(lo, a, b, n);
        TEST_CHECK_MSG(memcmp(lo, full, (size_t)n * sizeof(mp_limb_t)) == 0,
                       "mullo low limbs match full product at threshold");

        lmmp_free(a);
        lmmp_free(b);
        lmmp_free(lo);
        lmmp_free(full);
    }
}

TEST_CASE("threshold/div", divide_threshold_boundary) {
    const mp_size_t ns[] = {DIV_DIVIDE_THRESHOLD > 1 ? DIV_DIVIDE_THRESHOLD - 1 : 1, DIV_DIVIDE_THRESHOLD,
                            DIV_DIVIDE_THRESHOLD + 1};
    u64 seed = 0x0123456789abcdefull;
    for (size_t n_index = 0; n_index < ARRAY_SIZE(ns); ++n_index) {
        mp_size_t n = ns[n_index];
        const mp_size_t na = 2 * n;
        mp_ptr num = alloc_limbs(na), den = alloc_limbs(n), q = alloc_limbs(n + 2),
               r = alloc_limbs(n + 2), t = alloc_limbs(2 * n + 4), qd = alloc_limbs(na + 2),
               sum = alloc_limbs(2 * n + 4);
        fill_deterministic(num, na, seed);
        fill_deterministic(den, n, seed ^ 0xfedcba9876543210ull);
        num[na - 1] &= ~((u64)1 << 63); /* 保证 q 不超过 n+1 limbs */

        lmmp_div_(q, r, num, na, den, n);
        // q*d + r == num（库乘重建 + 零垫对齐加）
        lmmp_mul_(qd, q, n + 1, den, n);
        lmmp_zero(sum, 2 * n + 4);
        memcpy(sum, qd, (size_t)(2 * n + 1) * 8);
        lmmp_zero(t, 2 * n + 4);
        memcpy(t, r, (size_t)n * 8);
        lmmp_add_n_(sum, sum, t, 2 * n + 4);
        TEST_CHECK_MSG(mpn_eq(sum, 2 * n + 4, num, na), "div q*d+r rebuild at boundary");
        // 余数界：r < den
        TEST_CHECK_MSG(mpn_cmp(r, n, den, n) < 0, "div rem < den at boundary");

        lmmp_free(num);
        lmmp_free(den);
        lmmp_free(q);
        lmmp_free(r);
        lmmp_free(t);
        lmmp_free(qd);
        lmmp_free(sum);
    }
}

TEST_CASE("threshold/divexact", divexact_basecase_boundaries) {
    u64 seed = 0x0fedcba987654321ull;
    typedef struct {
        mp_size_t nn, dn;
    } Case;
    /* 门两侧 ±1 × {divide 形状, mu 形状, 大 nn}；dn 再取梅森/缓存档 ±1 覆盖
       mu 块积的 plain/截断梅森/截断梅森缓存三路 */
    const mp_size_t dns[] = {DIVEXACT_BASECASE_THRESHOLD > 1 ? DIVEXACT_BASECASE_THRESHOLD - 1 : 1,
                             DIVEXACT_BASECASE_THRESHOLD, DIVEXACT_BASECASE_THRESHOLD + 1,
                             DIVEXACT_MERSENNE_THRESHOLD > 3 ? DIVEXACT_MERSENNE_THRESHOLD - 1 : 8,
                             DIVEXACT_MERSENNE_THRESHOLD + 1,
                             DIVEXACT_MERSENNE_CACHE_THRESHOLD > 3 ? DIVEXACT_MERSENNE_CACHE_THRESHOLD - 1 : 8,
                             DIVEXACT_MERSENNE_CACHE_THRESHOLD + 1};
    Case cases[21];
    size_t count = 0;
    for (size_t d_index = 0; d_index < ARRAY_SIZE(dns); ++d_index) {
        const mp_size_t dn = dns[d_index];
        cases[count++] = (Case){dn + dn / 2 + 2, dn};
        cases[count++] = (Case){4 * dn, dn};
        cases[count++] = (Case){4096 > 4 * dn ? 4096 : 4 * dn, dn};
    }
    for (size_t c_index = 0; c_index < ARRAY_SIZE(cases); ++c_index) {
        const Case c = cases[c_index];
        const mp_size_t qn = c.nn - c.dn;
        mp_ptr q0 = alloc_limbs(qn), dp = alloc_limbs(c.dn), np = alloc_limbs(c.nn + 2),
               dst = alloc_limbs(c.nn + 2);
        fill_deterministic(q0, qn, seed);
        fill_deterministic(dp, c.dn, seed ^ 0xa5a5a5a55a5a5a5aull);
        dp[0] |= 1u;
        dp[c.dn - 1] |= (u64)1 << 63;

        if (qn >= c.dn)
            lmmp_mul_(np, q0, qn, dp, c.dn);
        else
            lmmp_mul_(np, dp, c.dn, q0, qn);
        np[c.nn] = 0;

        lmmp_divexact_(dst, np, c.nn, dp, c.dn);
        TEST_CHECK_MSG(mpn_eq(dst, qn, q0, qn),
                       "divexact boundary recovers original quotient");

        lmmp_free(q0);
        lmmp_free(dp);
        lmmp_free(np);
        lmmp_free(dst);
    }
}

TEST_CASE("threshold/pow", pow_1_exp_boundary) {
    const mp_limb_t base = 3;
    mp_ptr b = alloc_limbs(1);
    b[0] = base;
    const ulong exps[] = {POW_1_EXP_THRESHOLD > 1 ? (ulong)POW_1_EXP_THRESHOLD - 1 : 1,
                        (ulong)POW_1_EXP_THRESHOLD,
                          (ulong)POW_1_EXP_THRESHOLD + 1, 1ul};
    for (size_t e_index = 0; e_index < ARRAY_SIZE(exps); ++e_index) {
        ulong e = exps[e_index];
        const mp_size_t rn = lmmp_pow_size_(b, 1, e);
        mp_ptr dst = alloc_limbs(rn + 2), dst1 = alloc_limbs(rn + 2);
        // 多 limb 入口（单 limb 底）与 pow_1_ 精确一致
        const mp_size_t got = lmmp_pow_(dst, rn, b, 1, e);
        const mp_size_t got1 = lmmp_pow_1_(dst1, rn, base, e);
        TEST_CHECK_MSG(got == got1 && memcmp(dst, dst1, (size_t)got * 8) == 0,
                       "pow_ vs pow_1_ at exp boundary");
        // 模幂参考（32 位素数）
        bool ok = true;
        for (int i = 0; i < MOD_P_CNT && ok; ++i)
            ok = mod32(dst, got, MOD_P[i]) == modpow32(base, e, MOD_P[i]);
        TEST_CHECK_MSG(ok, "pow boundary modpow check");
        lmmp_free(dst);
        lmmp_free(dst1);
    }
    lmmp_free(b);
}

void test_threshold_boundaries(void) {
    TEST_RUN("threshold/mul", mul_n_toom_boundaries);
    TEST_RUN("threshold/mullo", mullo_dc_boundary);
    TEST_RUN("threshold/div", divide_threshold_boundary);
    TEST_RUN("threshold/divexact", divexact_basecase_boundaries);
    TEST_RUN("threshold/pow", pow_1_exp_boundary);
}

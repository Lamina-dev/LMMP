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
#include "lmmp/impl/mul_hard.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lmmp_test_utils;

/*
    乘法家族验证网（无 BigInt oracle）。交叉验证铁律：参考算法与被测算法
    规模必须匹配——
      1. 小规模（≤ toom 域）：分派器 vs basecase 精确比对（O(n²) 参考在该
         规模是匹配的独立算法）+ n=1 经 mullh 精确 + 模检查 + 长度不变式
      2. FFT 域（≥ MUL_FFT_THRESHOLD / SQR_FFT_THRESHOLD）：mul_fft/sqr_fft
         （分派器所选）vs toom8h/sqr_toom8 直调（纯 Toom 递归，其子问题
         永不进 FFT：MAYBE_mul_toom8h=0、SQR_TOOM8_MAX<<阈值）——同规模
         两条独立算法族互比；禁用 basecase（大规模不匹配）
      3. 平方 == 自乘（sqr vs mul 分派交叉）、mullo == mul 截断
      4. 模检查：乘积在 32 位素数集下与输入模乘值一致（数学独立）
      5. 长度不变式：归一化输入的乘积长度 ∈ {na+nb-1, na+nb}
*/

namespace {

// 模检查 + 长度不变式
void check_prod_mod(const mp_limb_t* a, mp_size_t na, const mp_limb_t* b, mp_size_t nb,
                    const mp_limb_t* dst) {
    TEST_CHECK_MSG(mod_is_product(dst, na + nb, a, na, b, nb), "modular product check");
    mp_size_t pl = norm_len(dst, na + nb);
    TEST_CHECK(pl == na + nb || pl == na + nb - 1);
}

// 两条路径精确比对（有效长度 + 逐 limb）
bool limbs_same(const mp_limb_t* x, const mp_limb_t* y, mp_size_t n) {
    return std::memcmp(x, y, (size_t)n * sizeof(mp_limb_t)) == 0;
}

}  // namespace

TEST_CASE("mul/basecase", mul_basecase_mod_and_n1_exact) {
    u64 seed = 0x13579bdf2468ace0ull;
    for (size_t na = 1; na <= 40; ++na) {
        for (size_t nb = 1; nb <= na; ++nb) {
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb), dst = alloc_limbs(na + nb + 1);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
            lmmp_mul_basecase_(dst, a, na, b, nb);
            check_prod_mod(a, (mp_size_t)na, b, (mp_size_t)nb, dst);

            // n=1：与 mullh 精确比对
            if (na == 1 && nb == 1) {
                mp_limb_t ref[2];
                lmmp_mullh_(a[0], b[0], ref);
                TEST_CHECK_EQ(dst[0], ref[0]);
                TEST_CHECK_EQ(dst[1], ref[1]);
            }
            lmmp_free(a); lmmp_free(b); lmmp_free(dst);
        }
    }
    // 写死常数锚点：(2^64-1)^2（低 2 limbs）与 (2^63+1)*(2^64-1)
    mp_ptr a = alloc_limbs(1), b = alloc_limbs(1), d = alloc_limbs(3);
    a[0] = ~(u64)0; b[0] = ~(u64)0;
    lmmp_mul_basecase_(d, a, 1, b, 1);
    TEST_CHECK_EQ(d[0], 1ull);
    TEST_CHECK_EQ(d[1], ~(u64)0 - 1);
    a[0] = ((u64)1 << 63) + 1;
    lmmp_mul_basecase_(d, a, 1, b, 1);
    // (2^63+1)*(2^64-1) = 2^127 + 2^63 - 1
    TEST_CHECK_EQ(d[0], 0x7FFFFFFFFFFFFFFFull);
    TEST_CHECK_EQ(d[1], 0x8000000000000000ull);
    lmmp_free(a); lmmp_free(b); lmmp_free(d);
}

TEST_CASE("mul/basecase", sqr_basecase_vs_mul) {
    u64 seed = 0x2468ace013579bdfull;
    for (size_t na = 1; na <= 40; ++na) {
        mp_ptr a = alloc_limbs(na), dst = alloc_limbs(2 * na + 1), ref = alloc_limbs(2 * na + 1);
        random_limbs(a, na, seed);
        lmmp_sqr_basecase_(dst, a, na);
        lmmp_mul_(ref, a, na, a, na);  // 交叉：平方 == 自乘
        TEST_CHECK(limbs_same(dst, ref, 2 * na));
        check_prod_mod(a, (mp_size_t)na, a, (mp_size_t)na, dst);
        lmmp_free(a); lmmp_free(dst); lmmp_free(ref);
    }
}

TEST_CASE("mul/dispatch", mul_dispatch_vs_basecase_cross) {
    // toom 域内分派器 vs basecase（匹配的独立参考；O(n²) 成本限规模）
    u64 seed = 0x0123456789abcdefull;
    const mp_size_t sizes[] = {
        1, 2, 5, 19, 20, 21, 30, 40, 64, 65, 66, 80, 100, 120,
        150, 180, 220, 300, 400, 580, 581, 582, 700, 900, 1200
    };
    for (mp_size_t na : sizes) {
        for (int t = 0; t < 3; ++t) {
            mp_size_t nb = (mp_size_t)((u64)na * (t == 0 ? 1 : (t == 1 ? 2 : 3)) / (t == 0 ? 1 : (t == 1 ? 3 : 4)));
            if (nb < 1) nb = 1;
            if (nb > na) nb = na;
            // basecase 为 O(n^2)，限制总规模。
            if ((u64)na * nb > 1200000ull) continue;

            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            mp_ptr dst = alloc_limbs(na + nb + 2), ref = alloc_limbs(na + nb + 2);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
            lmmp_mul_(dst, a, na, b, nb);
            lmmp_mul_basecase_(ref, a, na, b, nb);
            TEST_CHECK(limbs_same(dst, ref, na + nb));
            check_prod_mod(a, na, b, nb, dst);
            lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(ref);
        }
    }
}

TEST_CASE("mul/fft", mul_fft_vs_toom8h) {
    // FFT 域（n >= MUL_FFT_THRESHOLD 分派器选 FFT）：与纯 Toom 递归直调
    // toom8h 互比（同规模独立算法族；basecase 在该规模不匹配，禁用）。
    // toom8h 契约：nb>=86、na<=4nb（原生比例含平衡与 4:1 不平衡）。
    u64 seed = 0xf1f2f3f4f5f6f7f8ull;
    struct { mp_size_t na, nb; } sizes[] = {
        {2345, 2345}, {2400, 1200}, {2400, 800}, {2400, 601},
        {3072, 3072}, {4096, 1024}, {5000, 1500}, {6000, 6000},
    };
    for (auto& s : sizes) {
        mp_ptr a = alloc_limbs(s.na), b = alloc_limbs(s.nb);
        mp_ptr dst = alloc_limbs(s.na + s.nb + 2), ref = alloc_limbs(s.na + s.nb + 2);
        random_limbs(a, s.na, seed);
        random_limbs(b, s.nb, seed);
        lmmp_mul_(dst, a, s.na, b, s.nb);               // 分派器 → FFT
        lmmp_mul_toom8h_(ref, a, s.na, b, s.nb);        // 纯 Toom 递归
        TEST_CHECK_MSG(limbs_same(dst, ref, s.na + s.nb), "fft vs toom8h");
        check_prod_mod(a, s.na, b, s.nb, dst);
        lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(ref);
    }
}

TEST_CASE("mul/fft", sqr_fft_vs_toom8) {
    // FFT 域平方（n >= SQR_FFT_THRESHOLD）：sqr_（→ sqr_fft）vs sqr_toom8_
    // 直调（SQR_TOOM8_MAX ≈ 195 << SQR_FFT_THRESHOLD，子问题永不进 FFT）
    u64 seed = 0xe1e2e3e4e5e6e7e8ull;
    for (mp_size_t n : {1544, 1600, 2048, 3072, 5000}) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(2 * n + 2), ref = alloc_limbs(2 * n + 2);
        random_limbs(a, n, seed);
        lmmp_sqr_(dst, a, n);
        lmmp_sqr_toom8_(ref, a, n);
        TEST_CHECK_MSG(limbs_same(dst, ref, 2 * n), "sqr_fft vs sqr_toom8");
        check_prod_mod(a, n, a, n, dst);
        lmmp_free(a); lmmp_free(dst); lmmp_free(ref);
    }
}

TEST_CASE("mul/hard_threshold_boundary", mul_hard_boundary_vs_basecase) {
    /* 覆盖硬编码函数(hardcoded n=1..19)经 toom 递归的边界尺寸:
       toom22 递归子规模 n/2 恰好落在 19/20 两侧 */
    u64 seed = 0x0f1e2d3c4b5a6978ull;
    const mp_size_t sizes[] = {
        19, 20, 21, 22, 37, 38, 39, 40, 41, 42, 63, 64, 65, 66
    };
    for (mp_size_t n : sizes) {
        for (int t = 0; t < 8; t++) {
            mp_ptr a = alloc_limbs(n), b = alloc_limbs(n);
            mp_ptr dst = alloc_limbs(2 * n + 2), ref = alloc_limbs(2 * n + 2);
            random_limbs(a, n, seed);
            random_limbs(b, n, seed);
            lmmp_mul_n_(dst, a, b, n);
            lmmp_mul_basecase_(ref, a, n, b, n);
            TEST_CHECK(limbs_same(dst, ref, 2 * n));
            check_prod_mod(a, n, b, n, dst);

            lmmp_sqr_(dst, a, n);
            lmmp_mul_(ref, a, n, a, n);  // sqr vs 自乘交叉
            TEST_CHECK(limbs_same(dst, ref, 2 * n));
            check_prod_mod(a, n, a, n, dst);
            lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(ref);
        }
    }
}

TEST_CASE("mul/hard_direct_all_sizes", mul_hard_direct_n1_to_n19) {
    /* 硬编码路径全规模直测: n=1..19 经顶层 lmmp_mul_n_/lmmp_sqr_ (阈值下
       必走硬编码调度) 对照 basecase 与模检查, 覆盖随机/全 1/全 0xFF/最高位等
       触发进位链极端的输入模式 */
    u64 seed = 0x0d1c2b3a49586775ull;
    for (mp_size_t n = 1; n <= LMMP_MUL_HARD_MAX_N; n++) {
        for (int pat = 0; pat < 6; pat++) {
            mp_ptr a = alloc_limbs(n), b = alloc_limbs(n);
            mp_ptr dst = alloc_limbs(2 * n + 2), ref = alloc_limbs(2 * n + 2);
            for (mp_size_t i = 0; i < n; i++) {
                switch (pat) {
                    case 0: a[i] = b[i] = xorshift64(seed); break;
                    case 1: a[i] = b[i] = ~(u64)0; break;
                    case 2: a[i] = b[i] = 1; break;
                    case 3: a[i] = b[i] = (u64)1 << 63; break;
                    case 4: a[i] = b[i] = (~(u64)0) - (u64)(i & 1); break;
                    default: a[i] = b[i] = xorshift64(seed) >> (i & 63); break;
                }
            }
            lmmp_mul_n_(dst, a, b, n);
            lmmp_mul_basecase_(ref, a, n, b, n);
            TEST_CHECK(limbs_same(dst, ref, 2 * n));
            // 模检查（pattern 输入可能非归一化，用值语义仍成立）
            TEST_CHECK(mod_is_product(dst, 2 * n, a, n, b, n));

            lmmp_sqr_(dst, a, n);
            lmmp_mul_basecase_(ref, a, n, a, n);
            TEST_CHECK(limbs_same(dst, ref, 2 * n));
            TEST_CHECK(mod_is_product(dst, 2 * n, a, n, a, n));
            lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(ref);
        }
    }
}

TEST_CASE("mul/unbalanced", mul_unbalanced_cross) {
    // 不平衡输入：toom 域用 basecase（匹配），FFT 域用 toom8h（匹配，
    // 契约 na<=4nb）；均附模检查
    u64 seed = 0xfedcba9876543210ull;
    struct Case { mp_size_t na, nb; };
    const Case toom_domain[] = {
        {600, 3}, {600, 5}, {600, 7}, {600, 10}, {600, 30}, {600, 100},
        {700, 100}, {800, 200}, {900, 250}, {1200, 300},
    };
    for (auto c : toom_domain) {
        if ((u64)c.na * c.nb > 1500000ull) continue;
        mp_ptr a = alloc_limbs(c.na), b = alloc_limbs(c.nb);
        mp_ptr dst = alloc_limbs(c.na + c.nb + 2), ref = alloc_limbs(c.na + c.nb + 2);
        random_limbs(a, c.na, seed);
        random_limbs(b, c.nb, seed);
        lmmp_mul_(dst, a, c.na, b, c.nb);
        lmmp_mul_basecase_(ref, a, c.na, b, c.nb);
        TEST_CHECK(limbs_same(dst, ref, c.na + c.nb));
        check_prod_mod(a, c.na, b, c.nb, dst);
        lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(ref);
    }
    const Case fft_domain[] = {
        {2316, 800}, {2400, 900}, {3116, 800}, {4096, 1024}, {6000, 1600},
    };
    for (auto c : fft_domain) {
        mp_ptr a = alloc_limbs(c.na), b = alloc_limbs(c.nb);
        mp_ptr dst = alloc_limbs(c.na + c.nb + 2), ref = alloc_limbs(c.na + c.nb + 2);
        random_limbs(a, c.na, seed);
        random_limbs(b, c.nb, seed);
        lmmp_mul_(dst, a, c.na, b, c.nb);             // 分派器（FFT 侧）
        lmmp_mul_toom8h_(ref, a, c.na, b, c.nb);      // Toom 侧（na<=4nb ✓）
        TEST_CHECK_MSG(limbs_same(dst, ref, c.na + c.nb), "unbalanced fft vs toom8h");
        check_prod_mod(a, c.na, b, c.nb, dst);
        lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(ref);
    }
}

TEST_CASE("mul/mullo", mullo_vs_mul_truncation) {
    u64 seed = 0xa53a591286afcbb5ull;
    for (mp_size_t n : {1, 2, 10, 50, 65, 83, 122, 201, 737, 1262}) {
        mp_ptr a = alloc_limbs(n), b = alloc_limbs(n), dst = alloc_limbs(n), full = alloc_limbs(2 * n + 2);
        random_limbs(a, n, seed);
        random_limbs(b, n, seed);
        lmmp_mul_(full, a, n, b, n);      // 全积参考（库路径）
        lmmp_mullo_(dst, a, b, n);        // 低 n limbs == 全积截断
        TEST_CHECK(limbs_same(dst, full, n));
        lmmp_free(a); lmmp_free(b); lmmp_free(dst); lmmp_free(full);
    }
}

TEST_CASE("mul/sqr_dispatch", sqr_dispatch_vs_mul) {
    // sqr vs 自乘（两条分派路径）至 FFT 域；toom 域附 basecase 精确
    u64 seed = 0x819821c27be91101ull;
    for (mp_size_t n : {1, 2, 19, 20, 21, 30, 65, 100, 400, 581, 800, 1200}) {
        mp_ptr a = alloc_limbs(n), dst = alloc_limbs(2 * n + 2), ref = alloc_limbs(2 * n + 2);
        random_limbs(a, n, seed);
        lmmp_sqr_(dst, a, n);
        lmmp_mul_(ref, a, n, a, n);
        TEST_CHECK(limbs_same(dst, ref, 2 * n));
        if ((u64)n * n <= 1500000ull) {
            lmmp_mul_basecase_(ref, a, n, a, n);
            TEST_CHECK(limbs_same(dst, ref, 2 * n));
        }
        lmmp_free(a); lmmp_free(dst); lmmp_free(ref);
    }
}

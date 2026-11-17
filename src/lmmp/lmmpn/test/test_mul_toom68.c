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

/* Toom-6.5 / Toom-8.5 乘法与 Toom-6/8 平方测试：
   1) 内核直调覆盖原生不平衡全比例范围（(p,q) 切分阶梯各档）
   2) basecase 精确对拍 + 模检查（独立路径；含负系数插值路径）
   3) 新阈值边界（MUL_TOOM6H/8H、SQR_TOOM6/8 的 ±1 经完整分派）
   4) 极端输入（全 1 / 全 FF） */

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

static void fill_pattern(mp_ptr p, size_t n, mp_limb_t pat) {
    for (size_t i = 0; i < n; ++i) p[i] = pat;
}

/* 与 basecase 精确对拍 + 模检查（两条独立实现路径；按契约有效长度） */
static void check_vs_oracle(const char* tag, mp_ptr a, mp_size_t na, mp_ptr b, mp_size_t nb, void (*fn)(mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t)) {
    mp_ptr dst = alloc_limbs(na + nb);
    mp_ptr ref = alloc_limbs(na + nb + 1);
    fn(dst, a, na, b, nb);
    lmmp_mul_basecase_(ref, a, na, b, nb);
    bool ok = memcmp(dst, ref, (size_t)(na + nb) * 8) == 0;
    if (!ok || !mod_is_product(dst, na + nb, a, na, b, nb)) TEST_CHECK_MSG(false, tag);
    else TEST_CHECK(true);
    lmmp_free(dst);
    lmmp_free(ref);
}

/* 内核直调 vs 独立分派路径（尺寸区间落在 toom44 及以下，路径独立） */
static void check_vs_dispatch(const char* tag, mp_ptr a, mp_size_t na, mp_ptr b, mp_size_t nb, void (*kfn)(mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t)) {
    mp_ptr r1 = alloc_limbs(na + nb);
    mp_ptr r2 = alloc_limbs(na + nb);
    kfn(r1, a, na, b, nb);
    lmmp_mul_(r2, a, na, b, nb);
    if (memcmp(r1, r2, (size_t)(na + nb) * sizeof(mp_limb_t)) != 0) TEST_CHECK_MSG(false, tag);
    else TEST_CHECK(true);
    lmmp_free(r1);
    lmmp_free(r2);
}

/* toom6h 原生比例范围：an/bn ∈ [1, 17/6)，an*3 < 8*bn 要求 bn < 46 时
   比例 < 8/3 —— 阶梯各档代表比例 */
const double kRatios6[] = {1.0, 1.05, 1.15, 1.3, 1.32, 1.45, 1.48, 1.8, 1.88, 2.0, 2.11, 2.4, 2.6, 2.82};
/* toom8h 原生比例范围：an/bn ∈ [1, 4] */
const double kRatios8[] = {1.0, 1.1, 1.22, 1.35, 1.5, 1.64, 1.8, 2.0, 2.24, 2.5, 2.85, 3.1, 3.5, 3.99};

static bool in_6h_range(mp_size_t na, mp_size_t nb) {
    /* 与内核契约一致：比例 < 17/6 且 nb>=46（原 8/3 宽限与 nb 42..45
       保证域外区间随契约收紧移除） */
    return na >= nb && nb >= 46 && 6 * na < 17 * nb;
}

TEST_CASE("mul/toom68", toom6h_kernel_full_range) {
    u64 seed = 0x6c6f666572746f6dull;
    int checked = 0;
    for (mp_size_t nb = 43; nb <= 92; ++nb) {
        for (size_t r_index = 0; r_index < ARRAY_SIZE(kRatios6); ++r_index) {
            double r = kRatios6[r_index];
            mp_size_t na = (mp_size_t)(nb * r);
            if (na < nb || !in_6h_range(na, nb)) continue;
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            random_limbs(a, na, &seed);
            random_limbs(b, nb, &seed);
            check_vs_dispatch("toom6h vs dispatch", a, na, b, nb, lmmp_mul_toom6h_);
            lmmp_free(a);
            lmmp_free(b);
            ++checked;
        }
    }
    TEST_CHECK_MSG(checked > 500, "覆盖数不足");
}

TEST_CASE("mul/toom68", toom8h_kernel_full_range) {
    u64 seed = 0x8a8f686967687475ull;
    int checked = 0;
    for (mp_size_t nb = 86; nb <= 140; ++nb) { /* 86=契约下界 */
        for (size_t r_index = 0; r_index < ARRAY_SIZE(kRatios8); ++r_index) {
            double r = kRatios8[r_index];
            mp_size_t na = (mp_size_t)(nb * r);
            if (na < nb || na > 4 * nb) continue;
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            random_limbs(a, na, &seed);
            random_limbs(b, nb, &seed);
            check_vs_dispatch("toom8h vs dispatch", a, na, b, nb, lmmp_mul_toom8h_);
            lmmp_free(a);
            lmmp_free(b);
            ++checked;
        }
    }
    TEST_CHECK_MSG(checked > 500, "覆盖数不足");
}

TEST_CASE("mul/toom68", sqr_toom6_kernel) {
    u64 seed = 0x72713a736972716eull;
    for (mp_size_t n = 26; n <= 120; ++n) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr r1 = alloc_limbs(2 * n), r2 = alloc_limbs(2 * n);
        random_limbs(a, n, &seed);
        lmmp_sqr_toom6_(r1, a, n);
        lmmp_sqr_(r2, a, n);
        if (memcmp(r1, r2, (size_t)(2 * n) * sizeof(mp_limb_t)) != 0) TEST_CHECK_MSG(false, "sqr_toom6 mismatch");
        else TEST_CHECK(true);
        lmmp_free(a);
        lmmp_free(r1);
        lmmp_free(r2);
    }
}

TEST_CASE("mul/toom68", sqr_toom8_kernel) {
    u64 seed = 0x33362e35392e3361ull;
    for (mp_size_t n = 51; n <= 160; ++n) {
        if (n == 57) continue; /* 契约排除的 s=1 退化点（16 点插值需 2s>3） */
        mp_ptr a = alloc_limbs(n);
        mp_ptr r1 = alloc_limbs(2 * n), r2 = alloc_limbs(2 * n);
        random_limbs(a, n, &seed);
        lmmp_sqr_toom8_(r1, a, n);
        lmmp_sqr_(r2, a, n);
        if (memcmp(r1, r2, (size_t)(2 * n) * sizeof(mp_limb_t)) != 0) TEST_CHECK_MSG(false, "sqr_toom8 mismatch");
        else TEST_CHECK(true);
        lmmp_free(a);
        lmmp_free(r1);
        lmmp_free(r2);
    }
}

TEST_CASE("mul/toom68", toom6h_oracle_balanced_and_unbalanced) {
    u64 seed = 0x21726f74616d6f6full;
    /* 平衡 + 阶梯代表比例（负系数插值路径由随机符号覆盖） */
    mp_size_t sizes[] = {43, 47, 55, 64, 73, 89};
    for (size_t nb_index = 0; nb_index < ARRAY_SIZE(sizes); ++nb_index) {
        mp_size_t nb = sizes[nb_index];
        const double r_values[] = {1.0, 1.32, 1.88, 2.82};
        for (size_t r_index = 0; r_index < ARRAY_SIZE(r_values); ++r_index) {
            double r = r_values[r_index];
            mp_size_t na = (mp_size_t)(nb * r);
            if (!in_6h_range(na, nb)) continue;
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            random_limbs(a, na, &seed);
            random_limbs(b, nb, &seed);
            check_vs_oracle("toom6h vs oracle", a, na, b, nb, lmmp_mul_toom6h_);
            lmmp_free(a);
            lmmp_free(b);
        }
    }
}

TEST_CASE("mul/toom68", toom8h_oracle_balanced_and_unbalanced) {
    u64 seed = 0x386f74206d6f6f6dull;
    mp_size_t sizes[] = {87, 95, 110, 130};
    for (size_t nb_index = 0; nb_index < ARRAY_SIZE(sizes); ++nb_index) {
        mp_size_t nb = sizes[nb_index];
        const double r_values[] = {1.0, 1.64, 2.85, 3.99};
        for (size_t r_index = 0; r_index < ARRAY_SIZE(r_values); ++r_index) {
            double r = r_values[r_index];
            mp_size_t na = (mp_size_t)(nb * r);
            if (na > 4 * nb) continue;
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            random_limbs(a, na, &seed);
            random_limbs(b, nb, &seed);
            check_vs_oracle("toom8h vs oracle", a, na, b, nb, lmmp_mul_toom8h_);
            lmmp_free(a);
            lmmp_free(b);
        }
    }
}

TEST_CASE("mul/toom68", thresholds_boundary_via_dispatch) {
    u64 seed = 0x7468726573686f6cull;
    /* 阈值 ±1 经 lmmp_mul_n_/lmmp_sqr_ 完整分派对拍 oracle */
    struct threshold_case {
        mp_size_t base;
        int is_sqr;
    } bounds[] = {
        {MUL_TOOM6H_THRESHOLD, 0},
        {MUL_TOOM8H_THRESHOLD, 0},
        {SQR_TOOM6_THRESHOLD, 1},
        {SQR_TOOM8_THRESHOLD, 1},
    };
    for (size_t bd_index = 0; bd_index < ARRAY_SIZE(bounds); ++bd_index) {
        const struct threshold_case bd = bounds[bd_index];
        for (int d = -1; d <= 1; ++d) {
            if (bd.base + (mp_size_t)d < 1) continue;
            mp_size_t n = bd.base + d;
            if (bd.is_sqr) {
                mp_ptr a = alloc_limbs(n);
                mp_ptr r = alloc_limbs(2 * n), ref = alloc_limbs(2 * n + 1);
                random_limbs(a, n, &seed);
                lmmp_sqr_(r, a, n);
                lmmp_mul_basecase_(ref, a, n, a, n);
                bool ok = memcmp(r, ref, (size_t)(2 * n) * 8) == 0;
                if (!ok || !mod_is_product(r, 2 * n, a, n, a, n))
                    TEST_CHECK_MSG(false, "sqr threshold boundary mismatch");
                else TEST_CHECK(true);
                lmmp_free(a);
                lmmp_free(r);
                lmmp_free(ref);
            } else {
                mp_ptr a = alloc_limbs(n), b = alloc_limbs(n);
                mp_ptr r = alloc_limbs(2 * n), ref = alloc_limbs(2 * n + 1);
                random_limbs(a, n, &seed);
                random_limbs(b, n, &seed);
                lmmp_mul_n_(r, a, b, n);
                lmmp_mul_basecase_(ref, a, n, b, n);
                bool ok = memcmp(r, ref, (size_t)(2 * n) * 8) == 0;
                if (!ok || !mod_is_product(r, 2 * n, a, n, b, n))
                    TEST_CHECK_MSG(false, "mul threshold boundary mismatch");
                else TEST_CHECK(true);
                lmmp_free(a);
                lmmp_free(b);
                lmmp_free(r);
                lmmp_free(ref);
            }
        }
    }
}

TEST_CASE("mul/toom68", extended_unbalanced_window_via_dispatch) {
    /* 扩窗路径：nb >= MUL_TOOM6H 且比例 < 4 时经 lmmp_mul_ 走 6h/8h
       （比例 < 17/6 走 6h，至 4 走 8h），oracle 对拍 */
    u64 seed = 0x6e7562616c616e63ull;
    mp_size_t nb0 = MUL_TOOM6H_THRESHOLD + 3;
    const double r_values[] = {1.0, 1.2, 1.32, 1.6, 2.0, 2.5, 2.82, 3.2, 3.99};
    for (size_t r_index = 0; r_index < ARRAY_SIZE(r_values); ++r_index) {
        double r = r_values[r_index];
        mp_size_t na = (mp_size_t)(nb0 * r);
        if (na >= 4 * nb0) na = 4 * nb0 - 1;
        mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb0);
        random_limbs(a, na, &seed);
        random_limbs(b, nb0, &seed);
        check_vs_oracle("extended window vs oracle", a, na, b, nb0, lmmp_mul_);
        lmmp_free(a);
        lmmp_free(b);
    }
}

TEST_CASE("mul/toom68", extreme_patterns) {
    u64 seed = 0x787472656d653031ull;
    /* 全 1 / 全 FF：进位链最长的病态输入 */
    struct limb_pattern {
        mp_limb_t pat;
    } pats[] = {{0x1111111111111111ull}, {LIMB_MAX}};
    for (size_t pt_index = 0; pt_index < ARRAY_SIZE(pats); ++pt_index) {
        const struct limb_pattern pt = pats[pt_index];
        const mp_size_t nb_values[] = {(mp_size_t)64, (mp_size_t)100};
        for (size_t nb_index = 0; nb_index < ARRAY_SIZE(nb_values); ++nb_index) {
            mp_size_t nb = nb_values[nb_index];
            const double r_values[] = {1.0, 2.0, 2.8};
            for (size_t r_index = 0; r_index < ARRAY_SIZE(r_values); ++r_index) {
                double r = r_values[r_index];
                mp_size_t na = (mp_size_t)(nb * r);
                if (!in_6h_range(na, nb)) continue;
                mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
                fill_pattern(a, na, pt.pat);
                fill_pattern(b, nb, pt.pat);
                check_vs_oracle("toom6h extreme", a, na, b, nb, lmmp_mul_toom6h_);
                lmmp_free(a);
                lmmp_free(b);
            }
        }
        const mp_size_t nb_values2[] = {(mp_size_t)104, (mp_size_t)160};
        for (size_t nb_index = 0; nb_index < ARRAY_SIZE(nb_values2); ++nb_index) {
            mp_size_t nb = nb_values2[nb_index];
            const double r_values2[] = {1.0, 2.5, 3.9};
            for (size_t r_index = 0; r_index < ARRAY_SIZE(r_values2); ++r_index) {
                double r = r_values2[r_index];
                mp_size_t na = (mp_size_t)(nb * r);
                if (na > 4 * nb) continue;
                mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
                fill_pattern(a, na, pt.pat);
                fill_pattern(b, nb, pt.pat);
                check_vs_oracle("toom8h extreme", a, na, b, nb, lmmp_mul_toom8h_);
                lmmp_free(a);
                lmmp_free(b);
            }
        }
        const mp_size_t n_values[] = {(mp_size_t)60, (mp_size_t)120};
        for (size_t n_index = 0; n_index < ARRAY_SIZE(n_values); ++n_index) {
            mp_size_t n = n_values[n_index];
            mp_ptr a = alloc_limbs(n);
            mp_ptr r1 = alloc_limbs(2 * n), r2 = alloc_limbs(2 * n);
            fill_pattern(a, n, pt.pat);
            lmmp_sqr_toom6_(r1, a, n);
            lmmp_sqr_(r2, a, n);
            if (memcmp(r1, r2, (size_t)(2 * n) * sizeof(mp_limb_t)) != 0) TEST_CHECK_MSG(false, "sqr6 extreme mismatch");
            else TEST_CHECK(true);
            lmmp_free(a);
            lmmp_free(r1);
            lmmp_free(r2);
        }
    }
    (void)seed;
}

void test_mul_toom68(void) {
    TEST_RUN("mul/toom68", toom6h_kernel_full_range);
    TEST_RUN("mul/toom68", toom8h_kernel_full_range);
    TEST_RUN("mul/toom68", sqr_toom6_kernel);
    TEST_RUN("mul/toom68", sqr_toom8_kernel);
    TEST_RUN("mul/toom68", toom6h_oracle_balanced_and_unbalanced);
    TEST_RUN("mul/toom68", toom8h_oracle_balanced_and_unbalanced);
    TEST_RUN("mul/toom68", thresholds_boundary_via_dispatch);
    TEST_RUN("mul/toom68", extended_unbalanced_window_via_dispatch);
    TEST_RUN("mul/toom68", extreme_patterns);
}

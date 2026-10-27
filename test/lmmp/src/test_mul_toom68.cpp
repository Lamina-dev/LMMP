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

/* Toom-6.5 / Toom-8.5 乘法与 Toom-6/8 平方测试：
   1) 内核直调覆盖原生不平衡全比例范围（(p,q) 切分阶梯各档）
   2) BigInt oracle 对拍（含负系数插值路径）
   3) 新阈值边界（MUL_TOOM6H/8H、SQR_TOOM6/8 的 ±1 经完整分派）
   4) 极端输入（全 1 / 全 FF） */

#include "lmmp/lmmpn.h"
#include "lmmp/impl/mparam.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0) p[n - 1] |= (u64)1 << 63;
}

void fill_pattern(mp_ptr p, size_t n, mp_limb_t pat) {
    for (size_t i = 0; i < n; ++i) p[i] = pat;
}

/* 与 BigInt oracle 对拍（按契约有效长度，勿用缓冲容量） */
void check_vs_oracle(const char* tag, mp_ptr a, mp_size_t na, mp_ptr b, mp_size_t nb, void (*fn)(mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t)) {
    mp_ptr dst = alloc_limbs(na + nb);
    fn(dst, a, na, b, nb);
    BigInt ba(a, na), bb(b, nb);
    BigInt expect = BigInt::mul_school(ba, bb);
    bool ok = expect.d.size() == (size_t)(na + nb);
    for (size_t i = 0; ok && i < expect.d.size(); ++i)
        if (expect.d[i] != dst[i]) ok = false;
    if (!ok) TEST_CHECK_MSG(false, tag);
    else TEST_CHECK(true);
    lmmp_free(dst);
}

/* 内核直调 vs 独立分派路径（尺寸区间落在 toom44 及以下，路径独立） */
void check_vs_dispatch(const char* tag, mp_ptr a, mp_size_t na, mp_ptr b, mp_size_t nb, void (*kfn)(mp_ptr, mp_srcptr, mp_size_t, mp_srcptr, mp_size_t)) {
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

bool in_6h_range(mp_size_t na, mp_size_t nb) {
    /* 与内核契约一致：比例 < 17/6 且 nb>=46（原 8/3 宽限与 nb 42..45
       保证域外区间随契约收紧移除） */
    return na >= nb && nb >= 46 && 6 * na < 17 * nb;
}

}  // namespace

TEST_CASE("mul/toom68", toom6h_kernel_full_range) {
    u64 seed = 0x6c6f666572746f6dull;
    int checked = 0;
    for (mp_size_t nb = 43; nb <= 92; ++nb) {
        for (double r : kRatios6) {
            mp_size_t na = (mp_size_t)(nb * r);
            if (na < nb || !in_6h_range(na, nb)) continue;
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
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
    for (mp_size_t nb = 87; nb <= 140; ++nb) {
        for (double r : kRatios8) {
            mp_size_t na = (mp_size_t)(nb * r);
            if (na < nb || na > 4 * nb) continue;
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
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
        random_limbs(a, n, seed);
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
        random_limbs(a, n, seed);
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
    for (mp_size_t nb : sizes) {
        for (double r : {1.0, 1.32, 1.88, 2.82}) {
            mp_size_t na = (mp_size_t)(nb * r);
            if (!in_6h_range(na, nb)) continue;
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
            check_vs_oracle("toom6h vs oracle", a, na, b, nb, lmmp_mul_toom6h_);
            lmmp_free(a);
            lmmp_free(b);
        }
    }
}

TEST_CASE("mul/toom68", toom8h_oracle_balanced_and_unbalanced) {
    u64 seed = 0x386f74206d6f6f6dull;
    mp_size_t sizes[] = {87, 95, 110, 130};
    for (mp_size_t nb : sizes) {
        for (double r : {1.0, 1.64, 2.85, 3.99}) {
            mp_size_t na = (mp_size_t)(nb * r);
            if (na > 4 * nb) continue;
            mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
            check_vs_oracle("toom8h vs oracle", a, na, b, nb, lmmp_mul_toom8h_);
            lmmp_free(a);
            lmmp_free(b);
        }
    }
}

TEST_CASE("mul/toom68", thresholds_boundary_via_dispatch) {
    u64 seed = 0x7468726573686f6cull;
    /* 阈值 ±1 经 lmmp_mul_n_/lmmp_sqr_ 完整分派对拍 oracle */
    struct {
        mp_size_t base;
        int is_sqr;
    } bounds[] = {
        {MUL_TOOM6H_THRESHOLD, 0},
        {MUL_TOOM8H_THRESHOLD, 0},
        {SQR_TOOM6_THRESHOLD, 1},
        {SQR_TOOM8_THRESHOLD, 1},
    };
    for (auto& bd : bounds) {
        for (int d = -1; d <= 1; ++d) {
            if (bd.base + (mp_size_t)d < 1) continue;
            mp_size_t n = bd.base + d;
            if (bd.is_sqr) {
                mp_ptr a = alloc_limbs(n);
                mp_ptr r = alloc_limbs(2 * n);
                random_limbs(a, n, seed);
                lmmp_sqr_(r, a, n);
                BigInt ba(a, n);
                BigInt expect = BigInt::sqr_school(ba);
                size_t sz = expect.d.size();
                bool ok = true;
                for (size_t i = 0; i < sz && ok; ++i)
                    if (expect.d[i] != r[i]) ok = false;
                if (!ok) TEST_CHECK_MSG(false, "sqr threshold boundary mismatch");
                else TEST_CHECK(true);
                lmmp_free(a);
                lmmp_free(r);
            } else {
                mp_ptr a = alloc_limbs(n), b = alloc_limbs(n);
                mp_ptr r = alloc_limbs(2 * n);
                random_limbs(a, n, seed);
                random_limbs(b, n, seed);
                lmmp_mul_n_(r, a, b, n);
                BigInt ba(a, n), bb(b, n);
                BigInt expect = BigInt::mul_school(ba, bb);
                size_t sz = expect.d.size();
                bool ok = true;
                for (size_t i = 0; i < sz && ok; ++i)
                    if (expect.d[i] != r[i]) ok = false;
                if (!ok) TEST_CHECK_MSG(false, "mul threshold boundary mismatch");
                else TEST_CHECK(true);
                lmmp_free(a);
                lmmp_free(b);
                lmmp_free(r);
            }
        }
    }
}

TEST_CASE("mul/toom68", extended_unbalanced_window_via_dispatch) {
    /* 扩窗路径：nb >= MUL_TOOM6H 且比例 < 4 时经 lmmp_mul_ 走 6h/8h
       （比例 < 17/6 走 6h，至 4 走 8h），oracle 对拍 */
    u64 seed = 0x6e7562616c616e63ull;
    mp_size_t nb0 = MUL_TOOM6H_THRESHOLD + 3;
    for (double r : {1.0, 1.2, 1.32, 1.6, 2.0, 2.5, 2.82, 3.2, 3.99}) {
        mp_size_t na = (mp_size_t)(nb0 * r);
        if (na >= 4 * nb0) na = 4 * nb0 - 1;
        mp_ptr a = alloc_limbs(na), b = alloc_limbs(nb0);
        random_limbs(a, na, seed);
        random_limbs(b, nb0, seed);
        check_vs_oracle("extended window vs oracle", a, na, b, nb0, lmmp_mul_);
        lmmp_free(a);
        lmmp_free(b);
    }
}

TEST_CASE("mul/toom68", extreme_patterns) {
    u64 seed = 0x787472656d653031ull;
    /* 全 1 / 全 FF：进位链最长的病态输入 */
    struct {
        mp_limb_t pat;
    } pats[] = {{0x1111111111111111ull}, {LIMB_MAX}};
    for (auto& pt : pats) {
        for (mp_size_t nb : {(mp_size_t)64, (mp_size_t)100}) {
            for (double r : {1.0, 2.0, 2.8}) {
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
        for (mp_size_t nb : {(mp_size_t)104, (mp_size_t)160}) {
            for (double r : {1.0, 2.5, 3.9}) {
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
        for (mp_size_t n : {(mp_size_t)60, (mp_size_t)120}) {
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

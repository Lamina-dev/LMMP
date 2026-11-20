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

/* Toom-9.3 不平衡乘法测试（平衡核入口已删除，机件由不平衡外层内部使用）：
   1) 缓存式不平衡路径覆盖单块、多块和尾块；
   2) 大尺寸与独立的 toom62 不平衡路径交叉；
   3) 分派带边缘形状走完整路由；
   4) 全 limb、全 FF 和分块顶 limb 进位构造。 */

#include "lmmp/lmmpn.h"
#include "mparam.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

static void fill_pattern(mp_ptr p, mp_size_t n, mp_limb_t pat) {
    for (mp_size_t i = 0; i < n; ++i)
        p[i] = pat;
}

static void check_vs_basecase(
    const char* tag,
    mp_ptr a,
    mp_size_t na,
    mp_ptr b,
    mp_size_t nb
) {
    mp_ptr dst = alloc_limbs(na + nb);
    mp_ptr ref = alloc_limbs(na + nb);
    lmmp_mul_toom93_unbalance_(dst, a, na, b, nb);
    lmmp_mul_basecase_(ref, a, na, b, nb);
    bool ok = memcmp(dst, ref, (size_t)(na + nb) * sizeof(mp_limb_t)) == 0;
    if (!ok || !mod_is_product(dst, na + nb, a, na, b, nb))
        TEST_CHECK_MSG(false, tag);
    else
        TEST_CHECK(true);
    lmmp_free(dst);
    lmmp_free(ref);
}

TEST_CASE("mul/toom93", unbalance_vs_basecase) {
    u64 seed = 0x93bba1a0ced00002ull;
    int checked = 0;
    for (mp_size_t nb = 20; nb <= 48; ++nb) {
        const mp_size_t nas[] = {3 * nb, 3 * nb + 1, 4 * nb, 6 * nb, 7 * nb + nb / 2};
        for (size_t i = 0; i < ARRAY_SIZE(nas); ++i) {
            mp_size_t na = nas[i];
            mp_ptr a = alloc_limbs(na);
            mp_ptr b = alloc_limbs(nb);
            random_limbs(a, na, &seed);
            random_limbs(b, nb, &seed);
            check_vs_basecase("toom93 unbalance", a, na, b, nb);
            lmmp_free(a);
            lmmp_free(b);
            ++checked;
        }
    }
    TEST_CHECK_MSG(checked > 140, "覆盖数不足");
}

TEST_CASE("mul/toom93", unbalance_vs_toom62) {
    u64 seed = 0x93c055a7b0000003ull;
    const mp_size_t nbs[] = {100, 200, 400, 700};
    const double ratios[] = {5.0, 5.5, 7.0, 9.0, 12.0};
    int checked = 0;
    for (size_t bi = 0; bi < ARRAY_SIZE(nbs); ++bi) {
        mp_size_t nb = nbs[bi];
        for (size_t ri = 0; ri < ARRAY_SIZE(ratios); ++ri) {
            mp_size_t na = (mp_size_t)(nb * ratios[ri]);
            mp_ptr a = alloc_limbs(na);
            mp_ptr b = alloc_limbs(nb);
            mp_ptr r1 = alloc_limbs(na + nb);
            mp_ptr r2 = alloc_limbs(na + nb);
            random_limbs(a, na, &seed);
            random_limbs(b, nb, &seed);
            lmmp_mul_toom93_unbalance_(r1, a, na, b, nb);
            lmmp_mul_toom62_unbalance_(r2, a, na, b, nb);
            bool ok = memcmp(r1, r2, (size_t)(na + nb) * sizeof(mp_limb_t)) == 0;
            if (!ok || !mod_is_product(r1, na + nb, a, na, b, nb))
                TEST_CHECK_MSG(false, "toom93 vs toom62");
            else
                TEST_CHECK(true);
            lmmp_free(a);
            lmmp_free(b);
            lmmp_free(r1);
            lmmp_free(r2);
            ++checked;
        }
    }
    TEST_CHECK_MSG(checked == (int)(ARRAY_SIZE(nbs) * ARRAY_SIZE(ratios)), "覆盖数不足");
}

/* 分派带边缘形状走 lmmp_mul_ 完整路由（3:1 下沿两侧与 4:1 上沿），
   与 basecase/模检查对拍 */
TEST_CASE("mul/toom93", contract_edge_dispatch) {
    u64 seed = 0x93ed6e5000000006ull;
    const mp_size_t nbs[] = {17, 31, 32, 33, 96, 459, 601, 602, 781};
    for (size_t bi = 0; bi < ARRAY_SIZE(nbs); ++bi) {
        mp_size_t nb = nbs[bi];
        mp_size_t n = 1 + (nb - 1) / 3;
        const mp_size_t nas[] = {8 * n, 9 * n + 1, 4 * nb};
        for (size_t i = 0; i < ARRAY_SIZE(nas); ++i) {
            mp_size_t na = nas[i];
            mp_ptr a = alloc_limbs(na);
            mp_ptr b = alloc_limbs(nb);
            mp_ptr dst = alloc_limbs(na + nb);
            mp_ptr ref = alloc_limbs(na + nb);
            random_limbs(a, na, &seed);
            random_limbs(b, nb, &seed);
            lmmp_mul_(dst, a, na, b, nb);
            lmmp_mul_basecase_(ref, a, na, b, nb);
            bool ok = memcmp(dst, ref, (size_t)(na + nb) * sizeof(mp_limb_t)) == 0;
            if (!ok || !mod_is_product(dst, na + nb, a, na, b, nb))
                TEST_CHECK_MSG(false, "toom93 edge dispatch");
            else
                TEST_CHECK(true);
            lmmp_free(a);
            lmmp_free(b);
            lmmp_free(dst);
            lmmp_free(ref);
        }
    }
}

TEST_CASE("mul/toom93", extreme_and_carry) {
    u64 seed = 0x93e07e0e00000004ull;
    const mp_limb_t patterns[] = {0x1111111111111111ull, LIMB_MAX};
    for (size_t pi = 0; pi < ARRAY_SIZE(patterns); ++pi) {
        mp_size_t nb = 40;
        mp_size_t na = 12 * nb;
        mp_ptr a = alloc_limbs(na);
        mp_ptr b = alloc_limbs(nb);
        fill_pattern(a, na, patterns[pi]);
        fill_pattern(b, nb, patterns[pi]);
        check_vs_basecase("toom93 extreme", a, na, b, nb);
        lmmp_free(a);
        lmmp_free(b);
    }

    for (int variant = 0; variant < 3; ++variant) {
        mp_size_t nb = 30;
        mp_size_t na = 11 * nb;
        mp_ptr a = alloc_limbs(na);
        mp_ptr b = alloc_limbs(nb);
        random_limbs(a, na, &seed);
        random_limbs(b, nb, &seed);
        mp_size_t n = 1 + (nb - 1) / 3;
        if (variant >= 1)
            b[n - 1] |= (mp_limb_t)1 << 63;
        if (variant >= 2)
            b[2 * n + (nb - 2 * n) - 1] |= (mp_limb_t)1 << 63;
        check_vs_basecase("toom93 carry", a, na, b, nb);
        lmmp_free(a);
        lmmp_free(b);
    }
}

void test_mul_toom93(void) {
    TEST_RUN("mul/toom93", unbalance_vs_basecase);
    TEST_RUN("mul/toom93", unbalance_vs_toom62);
    TEST_RUN("mul/toom93", contract_edge_dispatch);
    TEST_RUN("mul/toom93", extreme_and_carry);
}

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
#include "lmmp/mprand.h"
#include "lmmp/secret.h"
#include "lmmp/version.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <string.h>

TEST_CASE("rng/basic", seed_random_reproducible) {
    mp_size_t n = 16;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);

    // 相同种子和类型应可复现
    mp_size_t an = lmmp_seed_random_(a, n, 0x123456789ull, 0);
    mp_size_t bn = lmmp_seed_random_(b, n, 0x123456789ull, 0);
    TEST_CHECK_EQ(an, bn);
    for (mp_size_t i = 0; i < an; ++i) TEST_CHECK_EQ(a[i], b[i]);

    // 不同类型应产生不同序列（概率性，但相同实现下应稳定）
    mp_size_t cn = lmmp_seed_random_(b, n, 0x123456789ull, 1);
    bool diff = false;
    for (mp_size_t i = 0; i < an && i < cn; ++i) if (a[i] != b[i]) diff = true;
    TEST_CHECK_MSG(diff || an != cn, "seed_type changes sequence");

    lmmp_free(a);
    lmmp_free(b);
}

TEST_CASE("rng/basic", global_random_reproducible) {
    mp_size_t n = 16;
    mp_ptr a = alloc_limbs(n);
    mp_ptr b = alloc_limbs(n);

    lmmp_global_rng_init_(12345, 1);
    mp_size_t an = lmmp_random_(a, n);
    lmmp_global_rng_init_(12345, 1);
    mp_size_t bn = lmmp_random_(b, n);
    TEST_CHECK_EQ(an, bn);
    for (mp_size_t i = 0; i < an; ++i) TEST_CHECK_EQ(a[i], b[i]);

    lmmp_free(a);
    lmmp_free(b);
}

TEST_CASE("rng/strong", strong_random) {
    mp_size_t k = 20;
    mp_size_t n = 12;
    lmmp_strong_rng_t* rng = lmmp_strong_rng_init_(k, 42);
    TEST_CHECK_MSG(rng != NULL, "strong rng init");

    mp_ptr a = alloc_limbs(k);
    mp_ptr b = alloc_limbs(k);
    mp_size_t an = lmmp_strong_random_(a, n, rng);
    mp_size_t bn = lmmp_strong_random_(b, n, rng);
    TEST_CHECK_MSG(an > 0 && bn > 0, "strong random nonzero length");
    TEST_CHECK_MSG(an <= n && bn <= n, "strong random length bound");
    bool same = (an == bn);
    if (same) for (mp_size_t i = 0; i < an; ++i) if (a[i] != b[i]) same = false;
    TEST_CHECK_MSG(!same, "strong random sequence advances");

    lmmp_strong_rng_free_(rng);
    lmmp_free(a);
    lmmp_free(b);
}

TEST_CASE("rng/strong", strong_rng_extern_reproducible) {
    // 回归：扩容时新增状态曾未初始化（realloc 不清零，且初始化循环上界在更新 k 之前取值）
    const mp_size_t k1 = 8;
    const mp_size_t k2 = 24;

    lmmp_strong_rng_t* a = lmmp_strong_rng_init_(k1, 7);
    lmmp_strong_rng_extern_(a, k2);
    lmmp_strong_rng_t* b = lmmp_strong_rng_init_(k1, 7);
    lmmp_strong_rng_extern_(b, k2);

    mp_ptr ra = alloc_limbs(k2);
    mp_ptr rb = alloc_limbs(k2);
    mp_size_t na = lmmp_strong_random_(ra, k2, a);
    mp_size_t nb = lmmp_strong_random_(rb, k2, b);
    TEST_CHECK_EQ(na, nb);
    TEST_CHECK_MSG(na > 0, "extern grown rng nonzero output length");
    bool same = true;
    for (mp_size_t i = 0; i < na; ++i)
        if (ra[i] != rb[i]) same = false;
    TEST_CHECK_MSG(same, "extern growth deterministic");

    lmmp_strong_rng_free_(a);
    lmmp_strong_rng_free_(b);
    lmmp_free(ra);
    lmmp_free(rb);
}

void test_random(void) {
    TEST_RUN("rng/basic", seed_random_reproducible);
    TEST_RUN("rng/basic", global_random_reproducible);
    TEST_RUN("rng/strong", strong_random);
    TEST_RUN("rng/strong", strong_rng_extern_reproducible);
}

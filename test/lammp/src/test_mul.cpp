/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "lammp/lmmpn.h"
#include "lammp_test.hpp"
#include "lammp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lammp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0) p[n - 1] |= (u64)1 << 63;
}

void check_mul_result(const BigInt& expected, const mp_limb_t* got, mp_size_t na, mp_size_t nb) {
    size_t total = na + nb;
    size_t got_size = total;
    while (got_size > 0 && got[got_size - 1] == 0) --got_size;
    if (got_size == 0) got_size = 1;
    if (expected.d.size() != got_size) {
        TEST_CHECK_MSG(false, "mul result size mismatch");
        return;
    }
    for (size_t i = 0; i < got_size; ++i) {
        if (expected.d[i] != got[i]) {
            TEST_CHECK_MSG(false, "mul result limb mismatch");
            return;
        }
    }
    TEST_CHECK(true);
}

}  // namespace

TEST_CASE("mul/basecase", mul_basecase_vs_school) {
    u64 seed = 0x13579bdf2468ace0ull;
    for (size_t na = 1; na <= 40; ++na) {
        for (size_t nb = 1; nb <= na; ++nb) {
            mp_ptr a = alloc_limbs(na);
            mp_ptr b = alloc_limbs(nb);
            mp_ptr dst = alloc_limbs(na + nb + 1);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
            BigInt ba(a, na), bb(b, nb);
            BigInt expect = BigInt::mul_school(ba, bb);
            lmmp_mul_basecase_(dst, a, na, b, nb);
            check_mul_result(expect, dst, na, nb);
            lmmp_free(a); lmmp_free(b); lmmp_free(dst);
        }
    }
}

TEST_CASE("mul/sqr_basecase", sqr_basecase_vs_school) {
    u64 seed = 0x2468ace013579bdfull;
    for (size_t na = 1; na <= 40; ++na) {
        mp_ptr a = alloc_limbs(na);
        mp_ptr dst = alloc_limbs(2 * na + 1);
        random_limbs(a, na, seed);
        BigInt ba(a, na);
        BigInt expect = BigInt::sqr_school(ba);
        lmmp_sqr_basecase_(dst, a, na);
        check_mul_result(expect, dst, na, na);
        lmmp_free(a); lmmp_free(dst);
    }
}

TEST_CASE("mul/dispatch", mul_dispatch_vs_school) {
    u64 seed = 0x0123456789abcdefull;
    // 覆盖 basecase / toom22 / toom32 / toom33 / toom42 / toom43 / toom44 等阈值
    const mp_size_t sizes[] = {
        1, 2, 5, 19, 20, 21, 30, 40, 64, 65, 66, 80, 100, 120,
        150, 180, 220, 300, 400, 580, 581, 582, 700, 900, 1200, 1600, 2316
    };
    for (mp_size_t na : sizes) {
        for (int t = 0; t < 3; ++t) {
            mp_size_t nb = (mp_size_t)((u64)na * (t == 0 ? 1 : (t == 1 ? 2 : 3)) / (t == 0 ? 1 : (t == 1 ? 3 : 4)));
            if (nb < 1) nb = 1;
            if (nb > na) nb = na;
            // 学校乘法在过大尺寸下较慢，限制总规模。
            if ((u64)na * nb > 1200000ull) continue;

            mp_ptr a = alloc_limbs(na);
            mp_ptr b = alloc_limbs(nb);
            mp_ptr dst = alloc_limbs(na + nb + 2);
            random_limbs(a, na, seed);
            random_limbs(b, nb, seed);
            BigInt ba(a, na), bb(b, nb);
            BigInt expect = BigInt::mul_school(ba, bb);

            lmmp_mul_(dst, a, na, b, nb);
            check_mul_result(expect, dst, na, nb);
            lmmp_free(a); lmmp_free(b); lmmp_free(dst);
        }
    }
}

TEST_CASE("mul/unbalanced", mul_unbalanced_vs_school) {
    u64 seed = 0xfedcba9876543210ull;
    struct Case { mp_size_t na; mp_size_t nb; };
    const Case cases[] = {
        {600, 3}, {600, 5}, {600, 7}, {600, 10}, {600, 30}, {600, 100},
        {700, 100}, {800, 200}, {900, 250}, {1200, 300}, {1600, 400},
        {2316, 400}, {2316, 800}, {2400, 900}
    };
    for (auto c : cases) {
        if ((u64)c.na * c.nb > 1500000ull) continue;
        mp_ptr a = alloc_limbs(c.na);
        mp_ptr b = alloc_limbs(c.nb);
        mp_ptr dst = alloc_limbs(c.na + c.nb + 2);
        random_limbs(a, c.na, seed);
        random_limbs(b, c.nb, seed);
        BigInt ba(a, c.na), bb(b, c.nb);
        BigInt expect = BigInt::mul_school(ba, bb);
        lmmp_mul_(dst, a, c.na, b, c.nb);
        check_mul_result(expect, dst, c.na, c.nb);
        lmmp_free(a); lmmp_free(b); lmmp_free(dst);
    }
}

TEST_CASE("mul/mullo", mullo_vs_school) {
    u64 seed = 0xa5a5a5a5b5b5b5b5ull;
    for (mp_size_t n : {1, 2, 5, 10, 19, 20, 30, 50, 64, 65, 80, 120, 200, 400}) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr b = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(n);
        random_limbs(a, n, seed);
        random_limbs(b, n, seed);
        BigInt ba(a, n), bb(b, n);
        BigInt prod = BigInt::mul_school(ba, bb);
        if (prod.d.size() > (size_t)n) prod.d.resize(n);
        prod.trim();

        lmmp_mullo_(dst, a, b, n);
        TEST_CHECK(limb_vec_eq(prod, dst, n));
        lmmp_free(a); lmmp_free(b); lmmp_free(dst);
    }
}

TEST_CASE("mul/sqr_dispatch", sqr_dispatch_vs_school) {
    u64 seed = 0x1111111111111111ull;
    for (mp_size_t n : {1, 2, 5, 10, 19, 20, 21, 30, 64, 65, 66, 100, 200, 400, 580, 581, 582, 800, 1200, 2316}) {
        if ((u64)n * n > 1200000ull) continue;
        mp_ptr a = alloc_limbs(n);
        mp_ptr dst = alloc_limbs(2 * n + 2);
        random_limbs(a, n, seed);
        BigInt ba(a, n);
        BigInt expect = BigInt::sqr_school(ba);
        lmmp_sqr_(dst, a, n);
        check_mul_result(expect, dst, n, n);
        lmmp_free(a); lmmp_free(dst);
    }
}

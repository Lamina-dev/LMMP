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

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed, bool msb = true) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0 && msb) p[n - 1] |= (u64)1 << 63;
}

}  // namespace

TEST_CASE("numth/gcd", gcd_11) {
    u64 seed = 0x6d2b79f5ull;
    for (int i = 0; i < 500; ++i) {
        u64 a = xorshift64(seed) | 1;
        u64 b = xorshift64(seed) | 1;
        BigInt ba(a), bb(b);
        BigInt g = BigInt::gcd_euclid(ba, bb);
        TEST_CHECK_EQ(lmmp_gcd_11_(a, b), g.d[0]);
    }
    TEST_CHECK_EQ(lmmp_gcd_11_(1, 1), 1u);
    TEST_CHECK_EQ(lmmp_gcd_11_(UINT64_MAX, UINT64_MAX), UINT64_MAX);
}

TEST_CASE("numth/gcd", gcd_1) {
    u64 seed = 0x3c6ef372fe94f82bull;
    for (size_t un = 1; un <= 40; ++un) {
        for (int iter = 0; iter < 20; ++iter) {
            mp_ptr u = alloc_limbs(un);
            random_limbs(u, un, seed);
            u64 v = xorshift64(seed) | 1;
            BigInt bu(u, un), bv(v);
            BigInt g = BigInt::gcd_euclid(bu, bv);
            TEST_CHECK_EQ(lmmp_gcd_1_(u, un, v), g.d[0]);
            lmmp_free(u);
        }
    }
}

TEST_CASE("numth/gcd", gcd_22_and_gcd_2) {
    u64 seed = 0x9e3779b97f4a7c15ull;
    for (int iter = 0; iter < 200; ++iter) {
        mp_ptr u = alloc_limbs(2);
        mp_ptr v = alloc_limbs(2);
        mp_ptr dst = alloc_limbs(2);
        random_limbs(u, 2, seed);
        random_limbs(v, 2, seed);
        BigInt bu(u, 2), bv(v, 2);
        BigInt g = BigInt::gcd_euclid(bu, bv);
        mp_size_t gn = lmmp_gcd_22_(dst, u, v);
        TEST_CHECK_MSG(from_limbs(dst, gn) == g, "gcd_22 value");
        lmmp_free(u); lmmp_free(v); lmmp_free(dst);
    }

    for (int iter = 0; iter < 100; ++iter) {
        mp_size_t un = 3 + (iter % 40);
        mp_ptr u = alloc_limbs(un);
        mp_ptr v = alloc_limbs(2);
        mp_ptr dst = alloc_limbs(un);
        random_limbs(u, un, seed);
        random_limbs(v, 2, seed);
        BigInt bu(u, un), bv(v, 2);
        BigInt g = BigInt::gcd_euclid(bu, bv);
        mp_size_t gn = lmmp_gcd_2_(dst, u, un, v);
        TEST_CHECK_MSG(from_limbs(dst, gn) == g, "gcd_2 value");
        lmmp_free(u); lmmp_free(v); lmmp_free(dst);
    }
}

TEST_CASE("numth/gcd", gcd_basecase_vs_lehmer) {
    u64 seed = 0x5ac635d8aa3a93e7ull;
    for (mp_size_t n : {1, 2, 5, 10, 20, 40}) {
        for (int iter = 0; iter < 8; ++iter) {
            mp_size_t un = n + (iter % 5);
            mp_size_t vn = n > 1 ? n : 1;
            if (vn > un) vn = un;
            mp_ptr u = alloc_limbs(un);
            mp_ptr v = alloc_limbs(vn);
            mp_ptr d1 = alloc_limbs(un + vn);
            mp_ptr d2 = alloc_limbs(un + vn);
            random_limbs(u, un, seed);
            random_limbs(v, vn, seed);
            BigInt bu(u, un), bv(v, vn);
            BigInt g = BigInt::gcd_euclid(bu, bv);

            mp_size_t g1 = lmmp_gcd_basecase_(d1, u, un, v, vn);
            mp_size_t g2 = lmmp_gcd_lehmer_(d2, u, un, v, vn);
            TEST_CHECK_MSG(from_limbs(d1, g1) == g, "gcd_basecase value");
            TEST_CHECK_MSG(from_limbs(d2, g2) == g, "gcd_lehmer value");
            TEST_CHECK_EQ(g1, g2);
            lmmp_free(u); lmmp_free(v); lmmp_free(d1); lmmp_free(d2);
        }
    }
}

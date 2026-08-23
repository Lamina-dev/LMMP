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
#include "lammp/numth.h"
#include "lammp_test.hpp"
#include "lammp_test_utils.hpp"

#include <cstring>
#include <vector>

using namespace lammp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_limbs(mp_ptr p, size_t n, u64& seed, bool msb = true) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0 && msb) p[n - 1] |= (u64)1 << 63;
}

// 验证 dst * num 的低 n limbs 为 1。
void verify_inverse(const BigInt& num, const mp_limb_t* dst, mp_size_t n) {
    BigInt inv(dst, n);
    BigInt prod = BigInt::mul_school(num, inv);
    // 取低 n limbs，应等于 1
    BigInt low;
    low.d.resize(n);
    for (size_t i = 0; i < (size_t)n && i < prod.d.size(); ++i) low.d[i] = prod.d[i];
    low.trim();
    TEST_CHECK_MSG(low == BigInt(1), "inverse product low limbs == 1");
}

}  // namespace

TEST_CASE("numth/binvert", binvert_uint_ulong) {
    u64 seed = 0x123456789abcdef0ull;
    for (int i = 0; i < 500; ++i) {
        u32 a = (u32)(xorshift64(seed) | 1);
        u32 inv = lmmp_binvert_uint_(a);
        TEST_CHECK_MSG((uint)(a * inv) == 1u, "binvert_uint inverse");

        u64 b = xorshift64(seed) | 1;
        u64 inv64 = lmmp_binvert_ulong_(b);
        TEST_CHECK_MSG((u64)((u128)b * inv64) == 1, "binvert_ulong inverse");
    }
}

TEST_CASE("numth/binvert", binvert_2_3_4) {
    u64 seed = 0x1020304050607080ull;
    for (int i = 0; i < 300; ++i) {
        mp_limb_t a2[2], inv2[2];
        random_limbs(a2, 2, seed);
        a2[0] |= 1;
        lmmp_binvert_2_(inv2, a2);
        verify_inverse(BigInt(a2, 2), inv2, 2);

        mp_limb_t a3[3], inv3[3];
        random_limbs(a3, 3, seed);
        a3[0] |= 1;
        lmmp_binvert_3_(inv3, a3);
        verify_inverse(BigInt(a3, 3), inv3, 3);

        mp_limb_t a4[4], inv4[4];
        random_limbs(a4, 4, seed);
        a4[0] |= 1;
        lmmp_binvert_4_(inv4, a4);
        verify_inverse(BigInt(a4, 4), inv4, 4);
    }
}

TEST_CASE("numth/binvert", binvert_n_dc) {
    u64 seed = 0x0f1e2d3c4b5a6978ull;
    for (mp_size_t n : {1, 2, 3, 4, 5, 10, 20, 40, 80, 120}) {
        mp_ptr a = alloc_limbs(n);
        mp_ptr inv = alloc_limbs(n);
        mp_ptr tp = alloc_limbs((5 * n + 5) / 2);
        random_limbs(a, n, seed);
        a[0] |= 1;
        BigInt ba(a, n);
        lmmp_binvert_n_dc_(inv, a, n, tp);
        verify_inverse(ba, inv, n);
        lmmp_free(a); lmmp_free(inv); lmmp_free(tp);
    }
}

TEST_CASE("numth/binvert", binvert_unbalanced_variants) {
    u64 seed = 0xa1b2c3d4e5f60718ull;

    // unbalanced_1：单 limb 底数
    for (mp_size_t n : {2, 3, 5, 10, 30, 80}) {
        u64 a = xorshift64(seed) | 1;
        mp_ptr inv = alloc_limbs(n);
        lmmp_binvert_unbalanced_1_(inv, a, n);
        verify_inverse(BigInt(a), inv, n);
        lmmp_free(inv);
    }

    // unbalanced_2：双 limb 底数
    for (mp_size_t n : {3, 4, 10, 30, 80}) {
        mp_ptr a = alloc_limbs(2);
        mp_ptr inv = alloc_limbs(n);
        random_limbs(a, 2, seed);
        a[0] |= 1;
        BigInt ba(a, 2);
        lmmp_binvert_unbalanced_2_(inv, a, n);
        verify_inverse(ba, inv, n);
        lmmp_free(a); lmmp_free(inv);
    }

    // unbalanced 和通用入口
    for (mp_size_t n : {3, 5, 10, 30, 80}) {
        mp_size_t na = (n + 1) / 2;
        if (na < 1) na = 1;
        if (na >= n) na = n - 1;
        mp_ptr a = alloc_limbs(na);
        mp_ptr inv = alloc_limbs(n);
        mp_ptr tp = alloc_limbs((9 * na + 5) / 2 + 1);
        random_limbs(a, na, seed);
        a[0] |= 1;
        BigInt ba(a, na);
        lmmp_binvert_unbalanced_(inv, a, na, n, tp);
        verify_inverse(ba, inv, n);

        mp_ptr inv2 = alloc_limbs(n);
        lmmp_binvert_(inv2, a, na, n);
        verify_inverse(ba, inv2, n);
        lmmp_free(a); lmmp_free(inv); lmmp_free(inv2); lmmp_free(tp);
    }
}

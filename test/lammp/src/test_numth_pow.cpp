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

void random_limbs(mp_ptr p, size_t n, u64& seed) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0) p[n - 1] |= (u64)1 << 63;
}

}  // namespace

TEST_CASE("numth/pow", pow_1_variants) {
    struct { ulong base; ulong exp; } cases[] = {
        {1, 1}, {2, 63}, {3, 20}, {10, 18}, {0xf, 20},
        {0xff, 20}, {0xffff, 8}, {0xffffffffull, 5},
        {0x8000000000000000ull, 3}, {UINT64_MAX, 2}
    };
    for (auto c : cases) {
        BigInt expect = BigInt::pow(BigInt(c.base), c.exp);
        mp_size_t need = lmmp_pow_1_size_(c.base, c.exp);
        TEST_CHECK_MSG(need >= (mp_size_t)expect.d.size(), "pow_1_size enough");
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = lmmp_pow_1_(dst, need, c.base, c.exp);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "pow_1 value");
        lmmp_free(dst);
    }

    // 分派函数 u4/u8/u16/u32/u64
    auto check_dispatch = [&](ulong base, ulong exp, mp_size_t (*fn)(mp_ptr, mp_size_t, ulong, ulong)) {
        BigInt expect = BigInt::pow(BigInt(base), exp);
        mp_size_t need = lmmp_pow_1_size_(base, exp);
        mp_ptr dst = alloc_limbs((size_t)need + 2);
        mp_size_t rn = fn(dst, need, base, exp);
        TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "u*_pow_1 value");
        lmmp_free(dst);
    };
    check_dispatch(3, 20, lmmp_u4_pow_1_);
    check_dispatch(0xab, 20, lmmp_u8_pow_1_);
    check_dispatch(0xabcd, 10, lmmp_u16_pow_1_);
    check_dispatch(0xabcdef01ull, 7, lmmp_u32_pow_1_);
    check_dispatch(0x8000000000000000ull, 3, lmmp_u64_pow_1_);
}

TEST_CASE("numth/pow", pow_basecase_win2_pow) {
    u64 seed = 0xf0f0f0f0c3c3c3c3ull;
    for (mp_size_t n : {1, 2, 5, 10, 30, 100}) {
        for (int iter = 0; iter < 4; ++iter) {
            mp_ptr base = alloc_limbs(n);
            random_limbs(base, n, seed);
            BigInt bbase(base, n);
            ulong exps[] = {1, 2, 3, 5, 7, 17, 31, 63};
            for (ulong exp : exps) {
                BigInt expect = BigInt::pow(bbase, exp);
                mp_size_t need = lmmp_pow_size_(base, n, exp);
                TEST_CHECK_MSG(need >= (mp_size_t)expect.d.size(), "pow_size enough");
                mp_ptr dst = alloc_limbs((size_t)need + 2);

                mp_size_t rn = 0;
                if (exp == 1) {
                    rn = lmmp_pow_win2_(dst, need, base, n, exp);
                } else if (exp % 2 == 1 && exp >= 3 && exp <= 17) {
                    // basecase 只接受奇数次且底数>1；win2 均可
                    mp_ptr dst2 = alloc_limbs((size_t)need + 2);
                    mp_size_t rn2 = lmmp_pow_basecase_(dst2, need, base, n, exp);
                    TEST_CHECK_MSG(from_limbs(dst2, rn2) == expect, "pow_basecase value");
                    lmmp_free(dst2);
                    rn = lmmp_pow_win2_(dst, need, base, n, exp);
                } else {
                    rn = lmmp_pow_win2_(dst, need, base, n, exp);
                }
                TEST_CHECK_MSG(from_limbs(dst, rn) == expect, "pow_win2 value");

                mp_ptr dst3 = alloc_limbs((size_t)need + 2);
                mp_size_t rn3 = lmmp_pow_(dst3, need, base, n, exp);
                TEST_CHECK_MSG(from_limbs(dst3, rn3) == expect, "pow value");
                lmmp_free(dst); lmmp_free(dst3);
            }
            lmmp_free(base);
        }
    }
}

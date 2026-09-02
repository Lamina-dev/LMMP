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
#include "lmmp/mprand.h"
#include "lmmp/secret.h"
#include "lmmp/version.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

}  // namespace

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
    TEST_CHECK_MSG(rng != nullptr, "strong rng init");

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

TEST_CASE("hash/sanity", siphash_xxhash) {
    mp_limb_t in[8];
    u64 seed = 0x1234567890abcdefull;
    for (auto& x : in) x = xorshift64(seed);

    u64 key2[2] = {0x1111111111111111ull, 0x2222222222222222ull};
    u64 key1[1] = {0x3333333333333333ull};

    u64 h1 = lmmp_siphash24_(in, 8, key2);
    u64 h2 = lmmp_siphash24_(in, 8, key2);
    TEST_CHECK_EQ(h1, h2);

    u64 h3 = lmmp_siphash24_(in, 8, nullptr);
    TEST_CHECK_MSG(h3 != h1, "siphash key changes hash");

    u64 x1 = lmmp_xxhash_(in, 8, key1);
    u64 x2 = lmmp_xxhash_(in, 8, key1);
    TEST_CHECK_EQ(x1, x2);

    u64 x3 = lmmp_xxhash_(in, 8, nullptr);
    TEST_CHECK_MSG(x3 != x1, "xxhash key changes hash");

    // 空输入与 NULL key
    TEST_CHECK_MSG(lmmp_siphash24_(nullptr, 0, nullptr) != 0, "siphash empty nonzero");
    TEST_CHECK_MSG(lmmp_xxhash_(nullptr, 0, nullptr) != 0, "xxhash empty nonzero");

    // 变更输入应改变哈希
    in[0] ^= 1;
    TEST_CHECK_MSG(lmmp_xxhash_(in, 8, key1) != x1, "xxhash input change");
}

TEST_CASE("misc/version", version_strings) {
    const char* v = lmmp_get_version();
    const char* b = lmmp_get_build_type();
    TEST_CHECK_MSG(v != nullptr && std::strlen(v) > 0, "version string nonempty");
    TEST_CHECK_MSG(b != nullptr && std::strlen(b) > 0, "build type string nonempty");
}

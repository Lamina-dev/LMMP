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

#include <cstdint>
#include <cstring>

using namespace lmmp_test_utils;

namespace {

u64 mod_pow64(u64 a, u64 e, u64 mod) {
    u64 r = 1 % mod;
    a %= mod;
    while (e) {
        if (e & 1) r = (u64)((u128)r * a % mod);
        a = (u64)((u128)a * a % mod);
        e >>= 1;
    }
    return r;
}

bool ref_is_prime64(u64 n) {
    if (n < 2) return false;
    for (u64 p : {2ull, 3ull, 5ull, 7ull, 11ull, 13ull, 17ull, 19ull, 23ull, 29ull, 31ull, 37ull}) {
        if (n == p) return true;
        if (n % p == 0) return false;
    }
    u64 d = n - 1;
    int s = 0;
    while ((d & 1) == 0) { d >>= 1; ++s; }
    // 对 2^64 范围内确定性的 Miller-Rabin 基组
    for (u64 a : {2ull, 325ull, 9375ull, 28178ull, 450775ull, 9780504ull, 1795265022ull}) {
        if (a % n == 0) continue;
        u64 x = mod_pow64(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool composite = true;
        for (int r = 1; r < s; ++r) {
            x = (u64)((u128)x * x % n);
            if (x == n - 1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

bool ref_is_prime32(u32 n) {
    return ref_is_prime64(n);
}

}  // namespace

TEST_CASE("numth/prime", is_prime_uint_ulong_notrial) {
    u64 seed = 0xfeed0912eadc23aeull;
    for (u32 n = 0; n < 10000; ++n) {
        bool expect = ref_is_prime32(n);
        TEST_CHECK_MSG(lmmp_is_prime_uint_(n) == expect, "is_prime_uint small");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == expect, "is_prime_ulong small");
    }

    for (int i = 0; i < 10000; ++i) {
        u64 n = xorshift64(seed);
        bool expect = ref_is_prime64(n);
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == expect, "is_prime_ulong random");
        if (n > 2) TEST_CHECK_MSG(lmmp_is_prime_notrial_(n) == expect, "is_prime_notrial random");
    }
}

TEST_CASE("numth/prime", next_prev_prime) {
    // 小范围穷举
    ulong prev = 0;
    for (ulong n = 0; n < 2000; ++n) {
        ulong next = lmmp_next_prime_ulong_(n);
        if (n < 2) { TEST_CHECK_EQ(next, 2u); } else {
            bool found = false;
            for (ulong k = n + 1; k < 100000; ++k) {
                if (ref_is_prime64(k)) { TEST_CHECK_EQ(next, k); found = true; break; }
            }
            TEST_CHECK(found);
        }
        ulong p = lmmp_prev_prime_ulong_(n);
        if (n < 2) TEST_CHECK_EQ(p, 0u);
        else {
            bool found = false;
            for (ulong k = n; ; --k) {
                if (ref_is_prime64(k)) { TEST_CHECK_EQ(p, k); found = true; break; }
            }
            TEST_CHECK(found);
        }
        (void)prev;
    }

    // 随机 64 位附近
    u64 seed = 0x230bc3ae812873c0ull;
    for (int i = 0; i < 100; ++i) {
        ulong n = xorshift64(seed) | 1;
        if (n < 2) n = 2;
        ulong next = lmmp_next_prime_ulong_(n);
        TEST_CHECK_MSG(next > n, "next_prime > n");
        TEST_CHECK_MSG(ref_is_prime64(next), "next_prime is prime");
        if (next > n + 1) {
            for (ulong k = n + 1; k < next; ++k) TEST_CHECK_MSG(!ref_is_prime64(k), "next_prime no gap prime");
        }
        ulong p = lmmp_prev_prime_ulong_(n);
        if (p > 0) {
            TEST_CHECK_MSG(ref_is_prime64(p), "prev_prime is prime");
            TEST_CHECK_MSG(p <= n, "prev_prime <= n");
            for (ulong k = p + 1; k <= n; ++k) TEST_CHECK_MSG(!ref_is_prime64(k), "prev_prime no gap prime");
        }
    }
}

TEST_CASE("numth/prime", mulmod_powmod) {
    u64 seed = 0x2cbea9127ca8019full;
    for (int i = 0; i < 1000; ++i) {
        ulong mod = xorshift64(seed) | 3;  // 避免过小
        ulong a = xorshift64(seed) % mod;
        ulong b = xorshift64(seed) % mod;
        ulong q = 0;
        ulong r = lmmp_mulmod_ulong_(a, b, mod, &q);
        u128 prod = (u128)a * b;
        TEST_CHECK_MSG((u128)q * mod + r == prod, "mulmod relation");
        TEST_CHECK_MSG(r < mod, "mulmod rem bound");
    }

    for (int i = 0; i < 500; ++i) {
        u32 mod = (u32)(xorshift64(seed) | 3);
        u32 base = (u32)(xorshift64(seed) % mod);
        ulong exp = xorshift64(seed) & 0xffff;
        u32 r = lmmp_powmod_uint_odd_(base, exp, mod);
        u64 expect = mod_pow64(base, exp, mod);
        TEST_CHECK_EQ(r, (u32)expect);
    }

    for (int i = 0; i < 500; ++i) {
        ulong mod = xorshift64(seed) | 3;
        ulong base = xorshift64(seed) % mod;
        ulong exp = xorshift64(seed) & 0xffff;
        ulong r = lmmp_powmod_ulong_odd_(base, exp, mod);
        u64 expect = mod_pow64(base, exp, mod);
        TEST_CHECK_EQ(r, expect);
    }
}

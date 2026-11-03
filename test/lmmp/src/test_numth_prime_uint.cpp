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

/* 标量档素性（u32/u64）与 next/prev prime 验证：ref_is_prime64 为
   2^64 确定性 Miller-Rabin 标量参考（64 位内简单参考允许域），
   双向硬断言（不能错判）。128 位档见 test_numth_prime2.cpp，
   >128 位档见 test_numth_prime_n.cpp / test_numth_proth.cpp。 */

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_prime_ref.hpp"

using namespace lmmp_test_utils;
using namespace lmmp_test_prime_ref;

/*
    u32 档三免内核（is_prime_uint.c）定向测试：
    惰性剩余两档界（n<2^31 零条件减 / n>=2^31 每步条件减）、短表界、
    d 退化（n-1=2^t，ebits=1）、notrial 直调小素数（含 n==hash 基底路径）。
*/
TEST_CASE("numth/prime", is_prime_uint_ulong_notrial) {
    u64 seed = 0xfeed0912eadc23aeull;
    for (u32 n = 0; n < 10000; ++n) {
        bool expect = ref_is_prime32(n);
        TEST_CHECK_MSG(lmmp_is_prime_uint_(n) == expect, "is_prime_uint small");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == expect, "is_prime_ulong small");
    }

    for (int i = 0; i < 2000; ++i) {
        u64 n = xorshift64(seed);
        bool expect = ref_is_prime64(n);
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == expect, "is_prime_ulong random");
        if (n > 2) TEST_CHECK_MSG(lmmp_is_prime_notrial_(n) == expect, "is_prime_notrial random");
    }
}

TEST_CASE("numth/prime", is_prime_u32_tiers) {
    u64 seed = 0xc0ffee123456789ull;

    /* 2^31 档界两侧稠密扫描 */
    for (u32 n = 0x7ffffc00u; n <= 0x80000400u; n += 2) {
        u32 m = n | 1;
        bool expect = ref_is_prime32(m);
        TEST_CHECK_MSG(lmmp_is_prime_uint_(m) == expect, "is_prime_uint 2^31 tier");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(m) == expect, "is_prime_ulong 2^31 tier");
    }

    /* u32 顶部（n>=2^31 档上端；计数式循环避免 u32 回绕死循环） */
    for (u32 j = 0; j < 128; ++j) {
        u32 m = 0xffffff01u + 2 * j;
        bool expect = ref_is_prime32(m);
        TEST_CHECK_MSG(lmmp_is_prime_uint_(m) == expect, "is_prime_uint top tier");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(m) == expect, "is_prime_ulong top tier");
    }

    /* 短表界与 next_prime 拐角 */
    for (u32 n = 65400; n <= 65700; ++n) {
        bool expect = ref_is_prime32(n);
        TEST_CHECK_MSG(lmmp_is_prime_uint_(n) == expect, "is_prime_uint short-table edge");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(n) == expect, "is_prime_ulong short-table edge");
    }

    /* d 退化：n-1 = 2^t（ebits=1），含 Fermat 素数 3/5/17/257/65537 与合数 2^31+1 */
    for (u32 t = 1; t <= 31; ++t) {
        u32 m = (1u << t) + 1;
        bool expect = ref_is_prime32(m);
        TEST_CHECK_MSG(lmmp_is_prime_uint_(m) == expect, "is_prime_uint d=1");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(m) == expect, "is_prime_ulong d=1");
    }

    /* notrial 直调小奇数（覆盖 n==hash 基底、b>=n 起步路径；契约域 n>2 奇） */
    for (u32 m = 3; m <= 3001; m += 2) {
        bool expect = ref_is_prime32(m);
        TEST_CHECK_MSG(lmmp_is_prime_notrial_(m) == expect, "notrial small direct");
    }

    /* 随机 u32 大样本 + 档界邻域加密 */
    for (int i = 0; i < 20000; ++i) {
        u32 m = (u32)(xorshift64(seed) >> (i % 2 ? 32 : 0));
        if (i % 3 == 0) m = 0x7fffffffu - (m & 0xffffu);
        if (i % 3 == 1) m = 0xffffffffu - (m & 0xffffu);
        m |= 1;
        if (m < 3) m = 3;
        bool expect = ref_is_prime32(m);
        TEST_CHECK_MSG(lmmp_is_prime_uint_(m) == expect, "is_prime_uint random u32");
        TEST_CHECK_MSG(lmmp_is_prime_ulong_(m) == expect, "is_prime_ulong random u32");
        TEST_CHECK_MSG(lmmp_is_prime_notrial_(m) == expect, "notrial random u32");
    }

    /* next_prime u32 区段拐角：返回值与间隔无素数 */
    const u32 starts[] = {65521, 65522, 0x7ffffff0u, 0xfffff000u};
    for (u32 s : starts) {
        u32 p = (u32)lmmp_next_prime_ulong_(s);
        TEST_CHECK_MSG(p > s && ref_is_prime32(p), "next_prime u32 corner hit");
        for (u32 m = (s + 1) | 1; m < p; m += 2)
            TEST_CHECK_MSG(!ref_is_prime32(m), "next_prime u32 corner gap");
    }
}

TEST_CASE("numth/prime", next_prev_prime) {
    // 小范围穷举
    for (ulong n = 0; n < 1200; ++n) {
        ulong next = lmmp_next_prime_ulong_(n);
        if (n < 2) {
            TEST_CHECK_EQ(next, 2u);
        } else {
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
            for (ulong k = n;; --k) {
                if (ref_is_prime64(k)) { TEST_CHECK_EQ(p, k); found = true; break; }
            }
            TEST_CHECK(found);
        }
    }

    // 随机 64 位附近
    u64 seed = 0x230bc3ae812873c0ull;
    for (int i = 0; i < 60; ++i) {
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
    for (int i = 0; i < 400; ++i) {
        ulong mod = xorshift64(seed) | 3;  // 避免过小且保证奇
        ulong a = xorshift64(seed) % mod;
        ulong b = xorshift64(seed) % mod;
        ulong q = 0;
        ulong r = lmmp_mulmod_ulong_(a, b, mod, &q);
        /* q*mod + r == a*b（mullh 独立重建全积与商积） */
        mp_limb_t prod[2], qm[2];
        lmmp_mullh_(a, b, prod);
        lmmp_mullh_(q, mod, qm);
        mp_limb_t lo = qm[0] + r;
        mp_limb_t cy = lo < qm[0] ? 1 : 0;
        mp_limb_t hi = qm[1] + cy;
        TEST_CHECK_MSG(hi == prod[1] && lo == prod[0], "mulmod relation");
        TEST_CHECK_MSG(r < mod, "mulmod rem bound");
    }

    for (int i = 0; i < 300; ++i) {
        u32 mod = (u32)(xorshift64(seed) | 3);
        u32 base = (u32)(xorshift64(seed) % mod);
        ulong exp = xorshift64(seed) & 0xffff;
        u32 r = lmmp_powmod_uint_odd_(base, exp, mod);
        TEST_CHECK_EQ(r, (u32)mod_pow64(base, exp, mod));
    }

    for (int i = 0; i < 300; ++i) {
        ulong mod = xorshift64(seed) | 3;
        ulong base = xorshift64(seed) % mod;
        ulong exp = xorshift64(seed) & 0xffff;
        ulong r = lmmp_powmod_ulong_odd_(base, exp, mod);
        TEST_CHECK_EQ(r, mod_pow64(base, exp, mod));
    }
}

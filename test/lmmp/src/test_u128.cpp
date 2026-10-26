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

#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstdint>
#include <vector>

using namespace lmmp_test_utils;

namespace {

// 独立参考实现：从最高位到最低位的逐位恢复长除法。
// 与 u128.hpp 的实现不共代码，用于交叉验证。
void divmod_ref(u128 a, u128 b, u128& q, u128& r) {
    q = u128(0);
    r = u128(0);
    if (b == u128(0)) return;  // 调用方保证除数非零
    for (int i = 127; i >= 0; --i) {
        r = (r << 1) | ((a >> (unsigned)i) & u128(1));
        if (r >= b) {
            r -= b;
            q |= u128(1) << (unsigned)i;
        }
    }
}

// 验证 128÷64 核心的欧几里得恒等式：q*d + r == n 且 r < d。
// 商/余数由带余除法唯一决定，故两条恒等式即完整正确性判据。
void check_divmod_by_u64(u64 n1, u64 n0, u64 d) {
    u64 q, r;
    uint128_t::divmod_by_u64(n1, n0, d, q, r);
    u128 n = ((u128)n1 << 64) | n0;
    TEST_CHECK_MSG((u128)q * d + r == n, "quotient*divisor+remainder == dividend");
    TEST_CHECK_MSG(r < d, "remainder < divisor");
}

const u64 D_BOUNDARY[] = {
    1,
    2,
    3,
    0x7FFFFFFFull,
    0x80000000ull,
    0x80000001ull,
    0xFFFFFFFFull,
    0x100000000ull,
    0x100000001ull,
    0x7FFFFFFFFFFFFFFFull,
    0x8000000000000000ull,
    0x8000000000000001ull,
    0xFFFFFFFFFFFFFFFEull,
    0xFFFFFFFFFFFFFFFFull,
};

const u64 WORD_BOUNDARY[] = {
    0,
    1,
    2,
    0x7FFFFFFFull,
    0x80000000ull,
    0x80000001ull,
    0xFFFFFFFFull,
    0x100000000ull,
    0x100000001ull,
    0x7FFFFFFFFFFFFFFFull,
    0x8000000000000000ull,
    0x8000000000000001ull,
    0xFFFFFFFFFFFFFFFEull,
    0xFFFFFFFFFFFFFFFFull,
};

}  // namespace

// 128÷64 核心：边界值全叉积（除数×被除数高/低部均取半字/全字边界）。
TEST_CASE("u128/div", by_u64_boundary_cross) {
    u64 seed = 0xfeedfacec0ffeeull;
    for (u64 d : D_BOUNDARY) {
        for (u64 n0 : WORD_BOUNDARY) {
            // 高部取边界值与 < d 的随机值。
            for (u64 n1 : WORD_BOUNDARY) {
                if (n1 >= d) continue;
                check_divmod_by_u64(n1, n0, d);
            }
            for (int k = 0; k < 4; ++k) {
                u64 n1 = xorshift64(seed) % d;  // d > 0
                check_divmod_by_u64(n1, n0, d);
            }
        }
    }
}

// 128÷64 核心：随机输入的恒等式验证。
TEST_CASE("u128/div", by_u64_random_identity) {
    u64 seed = 0xc0ffee123456789bull;
    for (int i = 0; i < 400000; ++i) {
        u64 d = xorshift64(seed);
        if (d == 0) d = 1;
        u64 n1 = xorshift64(seed) % d;
        u64 n0 = xorshift64(seed);
        check_divmod_by_u64(n1, n0, d);
    }
    // 高部贴近 d-1 的最坏情形（试商回退路径）。
    for (int i = 0; i < 100000; ++i) {
        u64 d = xorshift64(seed);
        if (d == 0) d = 1;
        u64 n1 = d - 1 - (xorshift64(seed) % 3);
        u64 n0 = xorshift64(seed);
        check_divmod_by_u64(n1, n0, d);
    }
}

// 通用 128÷128：与独立逐位参考实现交叉验证，覆盖单字除数快路径、
// 商超 64 位的两段分解路径与 >= 2^64 除数的位级回退路径。
TEST_CASE("u128/div", general_vs_reference) {
    u64 seed = 0x0ddba11cafe42deadull;
    for (int i = 0; i < 30000; ++i) {
        u128 a = ((u128)xorshift64(seed) << 64) | xorshift64(seed);
        u128 b = ((u128)xorshift64(seed) << 64) | xorshift64(seed);
        int bbits = (int)(xorshift64(seed) % 128) + 1;  // 控制除数位数 1..128
        b >>= (unsigned)(128 - bbits);
        if (b == u128(0)) b = u128(1);
        u128 q, r, qr, rr;
        uint128_t::divmod(a, b, q, r);
        divmod_ref(a, b, qr, rr);
        TEST_CHECK_MSG(q == qr, "quotient matches reference");
        TEST_CHECK_MSG(r == rr, "remainder matches reference");
        if (q != qr || r != rr) break;  // 失败后不必继续刷屏
    }
}

// 基本语义：双参构造 (high, low) 顺序、跨 64 位移位、移位 >= 128 归零。
TEST_CASE("u128/ops", ctor_shift_semantics) {
    u128 x(0x0123456789abcdefull, 0xfedcba9876543210ull);
    TEST_CHECK_EQ((u64)(x >> 64), 0x0123456789abcdefull);
    TEST_CHECK_EQ((u64)x, 0xfedcba9876543210ull);
    TEST_CHECK_EQ((u64)(x << 64), 0ull);
    u128 y = x << 64;
    TEST_CHECK_EQ((u64)(y >> 64), 0xfedcba9876543210ull);
    TEST_CHECK_EQ((u64)y, 0ull);
    TEST_CHECK((u128)(x << 128) == u128(0));
    TEST_CHECK((u128)(x >> 128) == u128(0));
    TEST_CHECK((u128)(x >> 127) == u128(0));
    TEST_CHECK_EQ((u64)(x >> 63), 0x02468acf13579bdfull);  // 右移 63 位跨半字
    TEST_CHECK(((x << 100) >> 100) == (x & u128(0xFFFFFFF)));  // 左移溢出位丢弃后可逆
    // 环绕乘法与加法回绕。
    u128 m = u128(0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull);
    TEST_CHECK((m + u128(1)) == u128(0));
    TEST_CHECK(((u128)0xFFFFFFFFull * 0xFFFFFFFFull) == (((u128)0xFFFFFFFEull << 32) | 1));
}

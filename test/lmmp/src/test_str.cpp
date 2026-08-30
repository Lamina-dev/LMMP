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
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace lmmp_test_utils;

namespace {

mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

void random_big(BigInt& a, size_t n, u64& seed) {
    a.d.resize(n);
    for (size_t i = 0; i < n; ++i) a.d[i] = xorshift64(seed);
    if (n > 0) a.d[n - 1] |= (u64)1 << 63;
    a.trim();
}

// 库的字符串约定：每个字节保存一个 digit 值（0..base-1），
// 顺序为低位在前（little-endian digit order），零值字符串长度为 0。
// 下面实现独立的参考转换，用于交叉验证库的 from_str / to_str。

std::vector<mp_byte_t> to_digit_bytes(const BigInt& x, int base) {
    std::vector<mp_byte_t> digits;
    if (x.is_zero()) return digits;
    BigInt t = x;
    while (!t.is_zero()) {
        u64 rem = 0;
        t = BigInt::div_small(t, (u64)base, rem);
        digits.push_back((mp_byte_t)rem);
    }
    return digits;
}

BigInt from_digit_bytes(const mp_byte_t* p, mp_size_t len, int base) {
    BigInt r(0);
    for (mp_size_t i = len; i-- > 0;) {
        r = BigInt::mul_school(r, BigInt(base));
        r = BigInt::add_small(r, p[i]);
    }
    return r;
}

void check_roundtrip(const BigInt& x, int base) {
    mp_size_t n = (mp_size_t)x.d.size();
    mp_ptr tmp = alloc_limbs(n + 2);

    // 先计算字符串长度（使用 NULL 查询最大长度，再按实际值查询）
    mp_size_t str_len = lmmp_to_str_len_(x.d.data(), n, base);
    mp_byte_t* str = (mp_byte_t*)lmmp_alloc((size_t)str_len + 8);
    mp_size_t actual = lmmp_to_str_(str, x.d.data(), n, base);
    TEST_CHECK_MSG(actual <= str_len, "to_str actual <= len");

    mp_size_t need = lmmp_from_str_len_(str, actual, base);
    mp_ptr back = alloc_limbs((size_t)need + 2);
    mp_size_t back_n = lmmp_from_str_(back, str, actual, base);
    BigInt got = from_limbs(back, back_n);
    TEST_CHECK_MSG(got == x, "string roundtrip equality");

    lmmp_free(tmp);
    lmmp_free(str);
    lmmp_free(back);
}

}  // namespace

TEST_CASE("str/roundtrip", to_str_from_str_roundtrip) {
    u64 seed = 0xe34590cd6b789abeful;
    int bases[] = {2, 8, 10, 16, 36, 62, 100, 256};
    for (int base : bases) {
        for (size_t n : {1, 2, 5, 10, 20, 40}) {
            BigInt x;
            random_big(x, n, seed);
            check_roundtrip(x, base);
        }
        // 零值
        check_roundtrip(BigInt(0), base);
        // 小值
        check_roundtrip(BigInt(1), base);
    }
}

TEST_CASE("str/decimal", decimal_vs_reference) {
    u64 seed = 0x0cadf00d0badf87dull;
    for (size_t n = 1; n <= 24; ++n) {
        BigInt x;
        random_big(x, n, seed);
        std::vector<mp_byte_t> digits = to_digit_bytes(x, 10);

        // 库 from_str 解析我们的十进制 digit 字节串
        mp_size_t need = lmmp_from_str_len_(digits.data(), (mp_size_t)digits.size(), 10);
        mp_ptr got = alloc_limbs((size_t)need + 2);
        mp_size_t gn = lmmp_from_str_(got, digits.data(), (mp_size_t)digits.size(), 10);
        TEST_CHECK_MSG(from_limbs(got, gn) == x, "from_str decimal vs reference");
        TEST_CHECK_MSG(from_digit_bytes(digits.data(), (mp_size_t)digits.size(), 10) == x,
                       "reference parser roundtrip");

        // 库 to_str 生成的字节串与参考一致
        mp_size_t want = lmmp_to_str_len_(x.d.data(), (mp_size_t)x.d.size(), 10);
        mp_byte_t* str = (mp_byte_t*)lmmp_alloc((size_t)want + 2);
        mp_size_t actual = lmmp_to_str_(str, x.d.data(), (mp_size_t)x.d.size(), 10);
        TEST_CHECK_EQ(actual, (mp_size_t)digits.size());
        if (actual == (mp_size_t)digits.size())
            TEST_CHECK_MSG(std::memcmp(str, digits.data(), (size_t)actual) == 0,
                           "to_str decimal vs reference");

        lmmp_free(got);
        lmmp_free(str);
    }
}

TEST_CASE("str/known", known_values) {
    auto check_value = [](const BigInt& x, const std::vector<mp_byte_t>& expect, int base) {
        mp_size_t n = (mp_size_t)x.d.size();
        mp_size_t want = lmmp_to_str_len_(x.d.data(), n, base);
        mp_byte_t* str = (mp_byte_t*)lmmp_alloc((size_t)want + 2);
        mp_size_t actual = lmmp_to_str_(str, x.d.data(), n, base);
        TEST_CHECK_EQ(actual, (mp_size_t)expect.size());
        if (actual == (mp_size_t)expect.size())
            TEST_CHECK_MSG(std::memcmp(str, expect.data(), (size_t)actual) == 0,
                           "known digit string match");
        lmmp_free(str);
    };

    check_value(BigInt(0), {}, 10);
    check_value(BigInt(1), {1}, 10);
    check_value(BigInt(10), {0, 1}, 10);
    check_value(BigInt(255), {5, 5, 2}, 10);
    check_value(BigInt(255), {1, 1, 1, 1, 1, 1, 1, 1}, 2);
    check_value(BigInt(255), {15, 15}, 16);
}

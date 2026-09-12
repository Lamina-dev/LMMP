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

#include <cctype>
#include <cstdio>
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
    u64 seed = 0xe3459cd6b789abeful;
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

// ===== 特化字符串转换（十六进制/十进制字符接口，低位 digit 在数组前端） =====

namespace {

// 顶部 limb 的有效位数
int limb_bits_of(u64 x) {
    int k = 0;
    while (x) {
        x >>= 1;
        ++k;
    }
    return k;
}

// 参考十六进制串：每 limb 按 "%016llx" 自高向低拼接成大端串，整体反转后
// 前 digits 个字符即低位在前的约定串
std::string ref_hex_string(const BigInt& x, mp_size_t* digits, bool upper) {
    size_t n = x.d.size();
    std::string big;
    char buf[24];
    for (size_t i = n; i-- > 0;) {
        std::snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)x.d[i]);
        big += buf;
    }
    std::string rev(big.rbegin(), big.rend());
    mp_size_t bits = (mp_size_t)(64 * (n - 1) + (x.is_zero() ? 0 : limb_bits_of(x.d[n - 1])));
    *digits = (bits + 3) / 4;
    rev.resize((size_t)*digits);
    if (upper)
        for (char& c : rev) c = (char)std::toupper((unsigned char)c);
    return rev;
}

void check_hex_one(const BigInt& x) {
    mp_size_t n = (mp_size_t)x.d.size();
    mp_size_t digits = 0;
    ref_hex_string(x, &digits, false);

    TEST_CHECK_EQ(lmmp_to_hex_len_(x.d.data(), n), digits);
    TEST_CHECK_EQ(lmmp_to_hex_len_(NULL, n), (mp_size_t)(n * 16));

    for (int upper = 0; upper <= 1; ++upper) {
        char* got = (char*)lmmp_alloc((size_t)digits + 8);
        mp_size_t gotn = lmmp_to_hex_(got, x.d.data(), n, (bool)upper);
        TEST_CHECK_EQ(gotn, digits);
        TEST_CHECK_MSG(std::memcmp(got, ref_hex_string(x, &digits, (bool)upper).data(),
                                   (size_t)digits) == 0,
                       "to_hex vs reference");
        if (!x.is_zero())
            TEST_CHECK_MSG(got[digits - 1] != '0', "to_hex no leading zero");

        // from_hex：大小写混合 + 末尾补 '0'（值前导零）
        TEST_CHECK_EQ(lmmp_from_hex_len_(got, gotn), n);

        std::string mixed((size_t)gotn, '\0');
        for (mp_size_t i = 0; i < gotn; ++i)
            mixed[(size_t)i] = (i & 1) ? (char)std::toupper((unsigned char)got[(size_t)i]) : got[(size_t)i];

        std::string padded = mixed + "00000";
        mp_size_t need = lmmp_from_hex_len_(padded.data(), (mp_size_t)padded.size());
        mp_ptr back = alloc_limbs((size_t)need + 2);
        mp_size_t backn = lmmp_from_hex_(back, padded.data(), (mp_size_t)padded.size());
        TEST_CHECK_EQ(backn, n);
        if (backn == n)
            TEST_CHECK_MSG(from_limbs(back, backn) == x, "hex roundtrip");
        lmmp_free(back);
        lmmp_free(got);
    }
}

void check_decimal_one(const BigInt& x) {
    mp_size_t n = (mp_size_t)x.d.size();

    // 参考：to_str_(base 10) 数字节 + lmmp_dec_to_chars_（同为低位在前）
    mp_size_t reflen = lmmp_to_str_len_(x.d.data(), n, 10);
    mp_byte_t* digits = (mp_byte_t*)lmmp_alloc((size_t)reflen + 8);
    mp_size_t refn = lmmp_to_str_(digits, x.d.data(), n, 10);
    lmmp_dec_to_chars_(digits, refn);

    mp_size_t dlen = lmmp_to_decimal_len_(x.d.data(), n);
    TEST_CHECK_MSG(dlen >= refn, "to_decimal_len upper bound");
    char* got = (char*)lmmp_alloc((size_t)dlen + 8);
    mp_size_t gotn = lmmp_to_decimal_(got, x.d.data(), n);
    TEST_CHECK_EQ(gotn, refn);
    if (gotn == refn)
        TEST_CHECK_MSG(std::memcmp(got, digits, (size_t)refn) == 0, "to_decimal vs to_str+chars");

    // from：对照 from_str_（数字节）
    std::vector<mp_byte_t> digit_vals((size_t)refn);
    for (mp_size_t i = 0; i < refn; ++i) digit_vals[(size_t)i] = (mp_byte_t)(digits[i] - '0');
    mp_size_t wlen = lmmp_from_str_len_(digit_vals.data(), refn, 10);
    mp_ptr want = alloc_limbs((size_t)wlen + 2);
    mp_size_t wantn = lmmp_from_str_(want, digit_vals.data(), refn, 10);

    std::string padded = std::string(got, (size_t)gotn) + "000";
    mp_size_t dneed = lmmp_from_decimal_len_(padded.data(), (mp_size_t)padded.size());
    TEST_CHECK_MSG(dneed >= wantn, "from_decimal_len upper bound");
    mp_ptr back = alloc_limbs((size_t)dneed + 2);
    mp_size_t backn = lmmp_from_decimal_(back, padded.data(), (mp_size_t)padded.size());
    TEST_CHECK_EQ(backn, wantn);
    if (backn == wantn)
        TEST_CHECK_MSG(std::memcmp(back, want, (size_t)wantn * 8) == 0, "from_decimal vs from_str");
    TEST_CHECK_MSG(from_limbs(back, backn) == x, "decimal roundtrip");

    lmmp_free(back);
    lmmp_free(want);
    lmmp_free(got);
    lmmp_free(digits);
}

}  // namespace

TEST_CASE("str/hex_spec", hex_specialized) {
    u64 seed = 0xfeedfacecafebeeful;
    for (size_t n : {1, 2, 3, 5, 17, 64, 100, 333}) {
        BigInt x;
        random_big(x, n, seed);
        check_hex_one(x);
    }
    // 顶部 limb 特殊值（nibble 边界）
    for (u64 top : {1ull, 0xfull, 0x10ull, 0xffull}) {
        BigInt x;
        random_big(x, 9, seed);
        x.d[8] = top;
        x.trim();
        check_hex_one(x);
    }
    // 零值与空串
    BigInt zero;
    TEST_CHECK_EQ(lmmp_to_hex_len_(zero.d.data(), 1), (mp_size_t)0);
    char buf[8];
    TEST_CHECK_EQ(lmmp_to_hex_(buf, zero.d.data(), 1, false), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_hex_len_("000", 3), (mp_size_t)0);
    mp_ptr dst = alloc_limbs(4);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "0000", 4), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "", 0), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "f", 1), (mp_size_t)1);
    TEST_CHECK_EQ(dst[0], (mp_limb_t)15);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "F", 1), (mp_size_t)1);
    TEST_CHECK_EQ(dst[0], (mp_limb_t)15);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "71", 2), (mp_size_t)1);
    TEST_CHECK_EQ(dst[0], (mp_limb_t)0x17);
    lmmp_free(dst);
}

TEST_CASE("str/decimal_spec", decimal_specialized) {
    u64 seed = 0xd1ce0dd1ce0dd1ceull;
    // 覆盖 basecase/分治阈值（30/45/100 limb 附近）与幂表逐层增长
    for (size_t n : {1, 2, 3, 29, 30, 31, 44, 45, 60, 100, 101, 137, 200, 300, 500, 800}) {
        BigInt x;
        random_big(x, n, seed);
        check_decimal_one(x);
    }
    // 幂表收缩复用：小规模再次转换
    for (int rep = 0; rep < 3; ++rep) {
        BigInt x;
        random_big(x, 33, seed);
        check_decimal_one(x);
    }
    // 顶部 limb 特殊值
    for (u64 top : {1ull, 0x10ull, 0xffull}) {
        BigInt x;
        random_big(x, 41, seed);
        x.d[40] = top;
        x.trim();
        check_decimal_one(x);
    }
    // 零值与空串
    BigInt zero;
    TEST_CHECK_MSG(lmmp_to_decimal_len_(zero.d.data(), 1) >= 1, "zero to_decimal_len");
    char buf[8];
    TEST_CHECK_EQ(lmmp_to_decimal_(buf, zero.d.data(), 1), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_decimal_len_("0000", 4), (mp_size_t)0);
    mp_ptr dst = alloc_limbs(4);
    TEST_CHECK_EQ(lmmp_from_decimal_(dst, "000000", 6), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_decimal_(dst, "", 0), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_decimal_(dst, "7", 1), (mp_size_t)1);
    TEST_CHECK_EQ(dst[0], (mp_limb_t)7);
    TEST_CHECK_EQ(lmmp_from_decimal_(dst, "96", 2), (mp_size_t)1);  // 低位在前: 96 -> 6*10+9=69
    TEST_CHECK_EQ(dst[0], (mp_limb_t)69);
    lmmp_free(dst);
}

TEST_CASE("str/decimal_powcache", decimal_pow_cache_lifecycle) {
    u64 seed = 0x5eed5eed5eed5eedull;
    BigInt big1, big2, small;
    random_big(big1, 300, seed);
    random_big(big2, 150, seed);
    random_big(small, 35, seed);

    check_decimal_one(big1);  // 增长
    check_decimal_one(small); // 复用
    check_decimal_one(big2);  // 复用

    // deinit 释放幂表后重新初始化，验证惰性重建
    lmmp_global_deinit();
    lmmp_global_init();
    check_decimal_one(big2);
    check_decimal_one(small);
}

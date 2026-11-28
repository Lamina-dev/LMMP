/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

/*
    字符串转换验证（无 BigInt oracle）。库约定：每字节一个 digit 值
    （0..base-1），低位在前（little-endian digit order），零值串长 0。

   1. round-trip：to_str → from_str 值复原（多进制 × 多尺寸）
   2. div_1 重算数字：被除数逐位 div_1 取余重建 digit 串，与 to_str
      输出精确一致（div_1 由 div 套件独立保证）
   3. Horner 重建：digit 串经 mul_1+加法逐位求值，与 from_str 输出
      精确一致
   4. 写死值锚点（0/1/10/255 等）
   5. 特化接口：hex（snprintf 按 limb 重建参考串）/ decimal（与
      to_str+dec_to_chars / from_str 交叉），幂表生命周期（deinit/init）
*/

#include "lmmp/lmmpn.h"
#include "cases.h"
#include "support/lmmp_test_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// 参考 digit 串：div_1 逐位取余（低位在前；独立于 to_str_ 的幂表路径）
static mp_size_t ref_digits(mp_byte_t *out, mp_srcptr x, mp_size_t n, int base) {
    mp_size_t len = 0;
    mp_ptr work = alloc_limbs(n + 1), q = alloc_limbs(n + 1);
    lmmp_zero(work, n + 1);
    memcpy(work, x, (size_t)n * 8);
    mp_size_t wn = norm_len(work, n + 1);
    while (wn > 0) {
        mp_limb_t r = lmmp_div_1_(q, work, wn, base);
        out[len++] = (mp_byte_t)r;
        memcpy(work, q, (size_t)wn * 8);
        while (wn > 0 && work[wn - 1] == 0) --wn;
    }
    lmmp_free(work);
    lmmp_free(q);
    return len;
}

// 参考 limb 值：Horner val = val*base + d（高位 digit 先；缓冲须已分配
// 且本函数全程保持零化）
static mp_size_t ref_value(mp_ptr val, mp_size_t cap, const mp_byte_t* d, mp_size_t len, int base) {
    lmmp_zero(val, cap);
    for (mp_size_t i = len; i-- > 0;) {
        lmmp_mul_1_(val, val, cap, base);  // 高位零垫，cap 内无溢出
        add_small_inplace(val, cap, d[i]);
    }
    return norm_len(val, cap);
}

// 顶部 limb 的有效位数
static int limb_bits_of(u64 x) {
    int k = 0;
    while (x) {
        x >>= 1;
        ++k;
    }
    return k;
}

// 参考十六进制串：每 limb "%016llx" 自高向低拼接后整体反转，前 digits
// 个字符即低位在前的约定串（纯 limb 运算，无大整数）
static mp_size_t ref_hex_string(char *out, mp_srcptr x, mp_size_t n, bool upper) {
    for (mp_size_t i = 0; i < n; ++i) {
        char buf[17];
        snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)x[i]);
        for (size_t j = 0; j < 16; ++j) {
            char c = buf[15 - j];
            out[16 * i + j] = upper ? (char)toupper((unsigned char)c) : c;
        }
    }
    mp_size_t nlen = norm_len(x, n);
    mp_size_t bits = nlen ? 64 * (nlen - 1) + limb_bits_of(x[nlen - 1]) : 0;
    return (bits + 3) / 4;
}

static void check_hex_one(mp_srcptr x, mp_size_t n) {
    char *reference = lmmp_alloc(16 * n + 1);
    mp_size_t digits = ref_hex_string(reference, x, n, false);

    TEST_CHECK_EQ(lmmp_to_hex_len_(x, n), digits);
    TEST_CHECK_EQ(lmmp_to_hex_len_(NULL, n), (mp_size_t)(n * 16));

    for (int upper = 0; upper <= 1; ++upper) {
        char* got = (char*)lmmp_alloc((size_t)digits + 8);
        mp_size_t gotn = lmmp_to_hex_(got, x, n, (bool)upper);
        TEST_CHECK_EQ(gotn, digits);
        ref_hex_string(reference, x, n, (bool)upper);
        TEST_CHECK_MSG(memcmp(got, reference, digits) == 0, "to_hex vs reference");
        if (norm_len(x, n) > 0) TEST_CHECK_MSG(got[digits - 1] != '0', "to_hex no leading zero");

        // from_hex：大小写混合 + 末尾补 '0'（值前导零）
        TEST_CHECK_EQ(lmmp_from_hex_len_(got, gotn), norm_len(x, n));

        char *padded = lmmp_alloc(gotn + 5);
        for (mp_size_t i = 0; i < gotn; ++i)
            padded[i] =
                (i & 1) ? (char)toupper((unsigned char)got[(size_t)i]) : got[(size_t)i];

        memset(padded + gotn, '0', 5);
        mp_size_t need = lmmp_from_hex_len_(padded, gotn + 5);
        mp_ptr back = alloc_limbs((size_t)need + 2);
        mp_size_t backn = lmmp_from_hex_(back, padded, gotn + 5);
        TEST_CHECK_MSG(mpn_eq(back, backn, x, n), "hex roundtrip");
        lmmp_free(back);
        lmmp_free(got);
        lmmp_free(padded);
    }
    lmmp_free(reference);
}

static void check_decimal_one(mp_srcptr x, mp_size_t n) {
    // 参考：to_str_(base 10) 数字节 + lmmp_dec_to_chars_（同为低位在前）
    mp_size_t reflen = lmmp_to_str_len_(x, n, 10);
    mp_byte_t* digits = (mp_byte_t*)lmmp_alloc((size_t)reflen + 8);
    mp_size_t refn = lmmp_to_str_(digits, x, n, 10);
    lmmp_dec_to_chars_(digits, refn);

    mp_size_t dlen = lmmp_to_decimal_len_(x, n);
    TEST_CHECK_MSG(dlen >= refn, "to_decimal_len upper bound");
    char* got = (char*)lmmp_alloc((size_t)dlen + 8);
    mp_size_t gotn = lmmp_to_decimal_(got, x, n);
    TEST_CHECK_EQ(gotn, refn);
    if (gotn == refn)
        TEST_CHECK_MSG(memcmp(got, digits, (size_t)refn) == 0, "to_decimal vs to_str+chars");

    // 独立数字位检查（大尺寸）：div_1 逐位参考不走 to_decimal 的幂表
    // 路径；got 与 rd 同为低位在前
    if (n >= 100) {
        mp_byte_t *rd = lmmp_alloc(64 * n);
        mp_size_t ref_digits_len = ref_digits(rd, x, n, 10);
        TEST_CHECK_EQ(ref_digits_len, gotn);
        for (mp_size_t i = 0; i < gotn && i < ref_digits_len; ++i)
            if (rd[(size_t)i] != (mp_byte_t)(got[i] - '0')) {
                TEST_CHECK_MSG(false, "to_decimal digit vs div_1 reference");
                break;
            }
        lmmp_free(rd);
    }

    // from：对照 from_str_（数字节）
    mp_byte_t *digit_vals = digits;
    for (mp_size_t i = 0; i < refn; ++i) digit_vals[(size_t)i] = (mp_byte_t)(digits[i] - '0');
    mp_size_t wlen = lmmp_from_str_len_(digit_vals, refn, 10);
    mp_ptr want = alloc_limbs((size_t)wlen + 2);
    mp_size_t wantn = lmmp_from_str_(want, digit_vals, refn, 10);

    char *padded = lmmp_alloc(gotn + 3);
    memcpy(padded, got, gotn);
    memset(padded + gotn, '0', 3);
    mp_size_t dneed = lmmp_from_decimal_len_(padded, gotn + 3);
    TEST_CHECK_MSG(dneed >= wantn, "from_decimal_len upper bound");
    mp_ptr back = alloc_limbs((size_t)dneed + 2);
    mp_size_t backn = lmmp_from_decimal_(back, padded, gotn + 3);
    TEST_CHECK_EQ(backn, wantn);
    if (backn == wantn)
        TEST_CHECK_MSG(memcmp(back, want, (size_t)wantn * 8) == 0, "from_decimal vs from_str");
    TEST_CHECK_MSG(mpn_eq(back, backn, x, n), "decimal roundtrip");

    lmmp_free(back);
    lmmp_free(want);
    lmmp_free(padded);
    lmmp_free(got);
    lmmp_free(digits);
}

TEST_CASE("str/roundtrip", to_str_from_str_roundtrip) {
    u64 seed = 0xe3459cd6b789abeful;
    int bases[] = {2, 8, 10, 16, 36, 62, 100, 256};
    for (size_t base_index = 0; base_index < ARRAY_SIZE(bases); ++base_index) {
        int base = bases[base_index];
        const mp_size_t n_values[] = {1, 2, 5, 10, 20, 40};
        for (size_t n_index = 0; n_index < ARRAY_SIZE(n_values); ++n_index) {
            mp_size_t n = n_values[n_index];
            for (int iter = 0; iter < 3; ++iter) {
                mp_ptr x = alloc_limbs(n);
                random_limbs(x, n, &seed);

                mp_size_t str_len = lmmp_to_str_len_(x, n, base);
                mp_byte_t* str = (mp_byte_t*)lmmp_alloc((size_t)str_len + 8);
                mp_size_t actual = lmmp_to_str_(str, x, n, base);
                TEST_CHECK_MSG(actual <= str_len, "to_str actual <= len");

                mp_size_t need = lmmp_from_str_len_(str, actual, base);
                mp_ptr back = alloc_limbs((size_t)need + 2);
                mp_size_t back_n = lmmp_from_str_(back, str, actual, base);
                TEST_CHECK_MSG(mpn_eq(back, back_n, x, n), "string roundtrip equality");

                // div_1 重算数字：digit 串精确一致
                mp_byte_t *rd = lmmp_alloc(64 * n);
                mp_size_t ref_digits_len = ref_digits(rd, x, n, base);
                bool dok = ref_digits_len == actual &&
                           memcmp(rd, str, (size_t)actual) == 0;
                TEST_CHECK_MSG(dok, "to_str digits == div_1 recomputation");
                lmmp_free(rd);

                // Horner 重建：from_str 值精确一致
                mp_ptr hv = alloc_limbs((size_t)need + 2);
                mp_size_t hn = ref_value(hv, need + 2, str, actual, base);
                TEST_CHECK_MSG(mpn_eq(hv, hn, x, n), "from_str value == Horner rebuild");

                lmmp_free(hv);
                lmmp_free(back);
                lmmp_free(str);
                lmmp_free(x);
            }
        }
        // 零值 round-trip（零值串长 0，from_str 空串回零）
        mp_ptr z = alloc_limbs(2);
        lmmp_zero(z, 2);
        {
            mp_byte_t zbuf[4];
            mp_size_t zn = lmmp_to_str_(zbuf, z, 2, base);
            TEST_CHECK_EQ(zn, (mp_size_t)0);
            mp_ptr back = alloc_limbs(2);
            mp_size_t bn = lmmp_from_str_(back, zbuf, zn, base);
            TEST_CHECK_MSG(bn == 0 || (bn == 1 && back[0] == 0), "zero roundtrip");
            lmmp_free(back);
        }
        lmmp_free(z);
    }
}

static void check_value(mp_srcptr x, mp_size_t n, const mp_byte_t *expect,
                        mp_size_t expected_len, int base) {
    mp_size_t want = lmmp_to_str_len_(x, n, base);
    mp_byte_t *str = lmmp_alloc(want + 8);
    mp_size_t actual = lmmp_to_str_(str, x, n, base);
    TEST_CHECK_EQ(actual, expected_len);
    if (actual == expected_len)
        TEST_CHECK_MSG(memcmp(str, expect, actual) == 0, "known digit string match");
    lmmp_free(str);
}

TEST_CASE("str/known", known_values) {
    const mp_limb_t v0[] = {0}, v1[] = {1}, v10[] = {10}, v255[] = {255};
    const mp_byte_t empty[] = {0};
    check_value(v0, 1, empty, 0, 10);
    check_value(v1, 1, (const mp_byte_t[]){1}, 1, 10);
    check_value(v10, 1, (const mp_byte_t[]){0, 1}, 2, 10);
    check_value(v255, 1, (const mp_byte_t[]){5, 5, 2}, 3, 10);
    check_value(v255, 1, (const mp_byte_t[]){1, 1, 1, 1, 1, 1, 1, 1}, 8, 2);
    check_value(v255, 1, (const mp_byte_t[]){15, 15}, 2, 16);

    const mp_limb_t vB[] = {0, 1};
    const mp_byte_t wantB[65] = {[64] = 1};
    check_value(vB, 2, wantB, ARRAY_SIZE(wantB), 2);

    const mp_limb_t vF[] = {LIMB_MAX};
    mp_byte_t wantF[16];
    memset(wantF, 15, sizeof(wantF));
    check_value(vF, 1, wantF, ARRAY_SIZE(wantF), 16);
}

TEST_CASE("str/hex_spec", hex_specialized) {
    u64 seed = 0xfeedfacecafebeeful;
    const mp_size_t n_values[] = {1, 2, 3, 5, 17, 64, 100, 333};
    for (size_t n_index = 0; n_index < ARRAY_SIZE(n_values); ++n_index) {
        mp_size_t n = n_values[n_index];
        mp_ptr x = alloc_limbs(n);
        random_limbs(x, n, &seed);
        check_hex_one(x, n);
        lmmp_free(x);
    }
    // 顶部 limb 特殊值（nibble 边界）
    const u64 top_values[] = {1ull, 0xfull, 0x10ull, 0xffull};
    for (size_t top_index = 0; top_index < ARRAY_SIZE(top_values); ++top_index) {
        u64 top = top_values[top_index];
        mp_ptr x = alloc_limbs(9);
        random_limbs(x, 9, &seed);
        x[8] = top;
        check_hex_one(x, 9);
        lmmp_free(x);
    }
    // 零值与空串
    mp_limb_t zero[1] = {0};
    TEST_CHECK_EQ(lmmp_to_hex_len_(zero, 1), (mp_size_t)0);
    char buf[8];
    TEST_CHECK_EQ(lmmp_to_hex_(buf, zero, 1, false), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_hex_len_("000", 3), (mp_size_t)0);
    mp_ptr dst = alloc_limbs(4);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "0000", 4), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "", 0), (mp_size_t)0);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "f", 1), (mp_size_t)1);
    TEST_CHECK_EQ(dst[0], (mp_limb_t)15);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "F", 1), (mp_size_t)1);
    TEST_CHECK_EQ(dst[0], (mp_limb_t)15);
    TEST_CHECK_EQ(lmmp_from_hex_(dst, "71", 2), (mp_size_t)1);
    TEST_CHECK_EQ(dst[0], (mp_limb_t)0x17);  // 低位在前：'7' 在前 = 0x7+0x10
    lmmp_free(dst);
}

TEST_CASE("str/decimal_spec", decimal_specialized) {
    u64 seed = 0xd1ce0dd1ce0dd1ceull;
    // 覆盖 basecase/分治阈值附近与幂表逐层增长
    const mp_size_t n_values[] = {1, 2, 3, 29, 30, 31, 44, 45, 60, 100, 137, 200, 300};
    for (size_t n_index = 0; n_index < ARRAY_SIZE(n_values); ++n_index) {
        mp_size_t n = n_values[n_index];
        mp_ptr x = alloc_limbs(n);
        random_limbs(x, n, &seed);
        check_decimal_one(x, n);
        lmmp_free(x);
    }
    // 幂表收缩复用：小规模再次转换
    for (int rep = 0; rep < 3; ++rep) {
        mp_ptr x = alloc_limbs(33);
        random_limbs(x, 33, &seed);
        check_decimal_one(x, 33);
        lmmp_free(x);
    }
    // 顶部 limb 特殊值
    const u64 top_values[] = {1ull, 0x10ull, 0xffull};
    for (size_t top_index = 0; top_index < ARRAY_SIZE(top_values); ++top_index) {
        u64 top = top_values[top_index];
        mp_ptr x = alloc_limbs(41);
        random_limbs(x, 41, &seed);
        x[40] = top;
        check_decimal_one(x, 41);
        lmmp_free(x);
    }
    // 零值与写死小值
    mp_limb_t zero[1] = {0};
    TEST_CHECK_MSG(lmmp_to_decimal_len_(zero, 1) >= 1, "zero to_decimal_len");
    char buf[8];
    TEST_CHECK_EQ(lmmp_to_decimal_(buf, zero, 1), (mp_size_t)0);
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
    mp_ptr big1 = alloc_limbs(300), big2 = alloc_limbs(150), small = alloc_limbs(35);
    random_limbs(big1, 300, &seed);
    random_limbs(big2, 150, &seed);
    random_limbs(small, 35, &seed);

    check_decimal_one(big1, 300);  // 增长
    check_decimal_one(small, 35);  // 复用
    check_decimal_one(big2, 150);  // 复用

    // deinit 释放幂表后重新初始化，验证首次使用时重新构建
    lmmp_global_deinit();
    lmmp_global_init();
    check_decimal_one(big2, 150);
    check_decimal_one(small, 35);
    lmmp_free(big1);
    lmmp_free(big2);
    lmmp_free(small);
}

void test_str(void) {
    TEST_RUN("str/roundtrip", to_str_from_str_roundtrip);
    TEST_RUN("str/known", known_values);
    TEST_RUN("str/hex_spec", hex_specialized);
    TEST_RUN("str/decimal_spec", decimal_specialized);
    TEST_RUN("str/decimal_powcache", decimal_pow_cache_lifecycle);
}

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

/* 整数幂家族验证（无 BigInt oracle；模幂见 test_numth_powmod.cpp）：
   1. 标量幂：pow_1 家族五入口互比（精确 limb）+ 32 位素数模幂参考
      （base^exp mod p，u64 乘积无溢出）+ 指数律 b^(e-1)*b、平方律
      b^(2k)==sqr(b^k)（库乘重建）+ 写死锚点（2^64、2^127、(2^64-1)^2、
      10^19 等）
   2. 多 limb 幂：pow_/pow_win2_/pow_basecase_ 三实现互比 + 模检查 +
      小指数乘法链重建（b^3==b*b*b）+ 位长区间
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

// 32 位素数模幂参考（p < 2^32，u64 中间乘积不溢出）
u64 modpow32(u64 base, u64 exp, u32 p) {
    u64 r = 1 % p, a = base % p;
    while (exp) {
        if (exp & 1) r = r * a % p;
        a = a * a % p;
        exp >>= 1;
    }
    return r;
}

// 顶部 limb 有效位数
int top_bits(u64 x) { return 64 - __builtin_clzll(x); }

// 标量幂全检：五入口与顶层 pow_1_ 精确一致 + 归一化长度 + 模幂参考
bool check_pow1_agree(ulong base, ulong exp, mp_size_t (*fn)(mp_ptr, mp_size_t, ulong, ulong)) {
    mp_size_t cap = lmmp_pow_1_size_(base, exp) + 2;
    mp_ptr d1 = alloc_limbs(cap), d2 = alloc_limbs(cap);
    mp_size_t r1 = fn(d1, cap, base, exp);
    mp_size_t r2 = lmmp_pow_1_(d2, cap, base, exp);
    bool ok = r1 == r2 && r1 >= 0 && r1 <= lmmp_pow_1_size_(base, exp);
    ok = ok && (r1 == 0 || d1[r1 - 1] != 0);
    ok = ok && memcmp(d1, d2, (size_t)r1 * 8) == 0;
    for (int i = 0; i < MOD_P_CNT && ok; ++i)
        ok = mod32(d1, r1, MOD_P[i]) == modpow32(base, exp, MOD_P[i]);
    lmmp_free(d1);
    lmmp_free(d2);
    return ok;
}

// 指数律重建：pow(b,e) == pow(b,e-1)*b 与 pow(b,2k) == sqr(pow(b,k))（库乘）
void check_pow1_laws(ulong base, ulong exp) {
    mp_size_t need = lmmp_pow_1_size_(base, exp);
    mp_ptr pe = alloc_limbs(need + 2), pm = alloc_limbs(need + 2);
    mp_size_t re = lmmp_pow_1_(pe, need, base, exp);
    if (exp >= 2) {
        mp_size_t rm = lmmp_pow_1_(pm, need, base, exp - 1);
        u64 hi = lmmp_mul_1_(pm, pm, rm, base);
        if (hi) pm[rm++] = hi;
        TEST_CHECK_MSG(mpn_eq(pe, re, pm, rm), "pow law b^e == b^(e-1)*b");
    }
    if (exp % 2 == 0) {
        mp_size_t rk = lmmp_pow_1_(pm, need, base, exp / 2);
        mp_ptr sq = alloc_limbs(2 * rk + 2);
        lmmp_sqr_(sq, pm, rk);
        TEST_CHECK_MSG(mpn_eq(pe, re, sq, 2 * rk), "pow law b^2k == sqr(b^k)");
        lmmp_free(sq);
    }
    lmmp_free(pe);
    lmmp_free(pm);
}

}  // namespace

TEST_CASE("numth/pow", pow_1_variants_and_anchors) {
    // 写死锚点（精确 limb 比对）
    struct { ulong base, exp; u64 w[3]; int n; } anchors[] = {
        {2, 64, {0, 1, 0}, 2},                                          // 2^64
        {2, 127, {0, 0x8000000000000000ull, 0}, 2},                     // 2^127
        {2, 128, {0, 0, 1}, 3},                                         // 2^128
        {(ulong)1 << 32, 2, {0, 1, 0}, 2},                              // (2^32)^2 = 2^64
        {10, 19, {0x8ac7230489e80000ull, 0, 0}, 1},                     // 10^19 < 2^64
        {1, 1000, {1, 0, 0}, 1},
    };
    for (const auto& a : anchors) {
        mp_ptr d = alloc_limbs(4);
        mp_size_t rn = lmmp_pow_1_(d, 4, a.base, a.exp);
        bool ok = rn == a.n && memcmp(d, a.w, (size_t)a.n * 8) == 0;
        lmmp_free(d);
        TEST_CHECK_MSG(ok, "pow_1 anchor");
    }
    // (2^64-1)^2 与 10^38：经 mullh 独立重建（全积原语）
    {
        mp_ptr d = alloc_limbs(3), prod = alloc_limbs(2);
        mp_size_t rn = lmmp_pow_1_(d, 3, UINT64_MAX, 2);
        lmmp_mullh_(UINT64_MAX, UINT64_MAX, prod);
        TEST_CHECK_MSG(mpn_eq(d, rn, prod, 2), "(2^64-1)^2 == mullh");
        u64 t19 = 0x8ac7230489e80000ull;
        rn = lmmp_pow_1_(d, 3, 10, 38);
        lmmp_mullh_(t19, t19, prod);
        TEST_CHECK_MSG(mpn_eq(d, rn, prod, 2), "10^38 == mullh(10^19)");
        lmmp_free(d);
        lmmp_free(prod);
    }

    // 域边界扫描（各族 2^k 边界 ±1、窗口进位形态、历史 bug 底数）
    static const ulong exps[] = {
        25, 26, 31, 32, 33, 47, 48, 63, 64, 100, 127, 128, 255, 256, 341, 511, 1024, 2000
    };
    int checked = 0;
    const ulong bases[] = {1,   2,        3,         6,          10,         15,
                           255, 256,      2760,      2761,       7386,       2642246,
                           50854530, 0x2bad99ul, 0xfffffffful, 0x100000000ul, 0xdeadbeefcafebul,
                           UINT64_MAX};
    for (ulong exp : exps) {
        for (ulong base : bases) {
            bool ok = check_pow1_agree(base, exp, lmmp_pow_1_);
            if (base <= 15) ok = ok && check_pow1_agree(base, exp, lmmp_u4_pow_1_);
            // u8/u16/u32 八进制梯子契约 base>=2（base=1 由 u4 档覆盖）
            if (base >= 2 && base <= 255) ok = ok && check_pow1_agree(base, exp, lmmp_u8_pow_1_);
            if (base >= 2 && base <= 0xffff) ok = ok && check_pow1_agree(base, exp, lmmp_u16_pow_1_);
            if (base >= 2 && base <= 0xfffffffful)
                ok = ok && check_pow1_agree(base, exp, lmmp_u32_pow_1_);
            if (base > 0xfffffffful) ok = ok && check_pow1_agree(base, exp, lmmp_u64_pow_1_);
            TEST_CHECK_MSG(ok, "pow_1 family agree");
            ++checked;
        }
    }
    TEST_CHECK_MSG(checked > 200, "sweep coverage");
    // 指数律抽样
    const ulong law_bases[] = {3, 10, 0x2bad99, 0xffffffff, UINT64_MAX - 1};
    for (ulong base : law_bases)
        for (ulong exp : {2ul, 3ul, 17ul, 64ul, 255ul}) check_pow1_laws(base, exp);
}

TEST_CASE("numth/pow", pow_multi_variants) {
    u64 seed = 0xf0f0f0f0c3c3c3c3ull;
    for (mp_size_t n : {1, 2, 5, 10, 30, 100}) {
        for (int iter = 0; iter < 3; ++iter) {
            mp_ptr base = alloc_limbs(n);
            random_limbs(base, n, seed);
            ulong exps[] = {1, 2, 3, 5, 7, 17, 31, 63};
            for (ulong exp : exps) {
                mp_size_t need = lmmp_pow_size_(base, n, exp);
                mp_ptr dst = alloc_limbs(need + 2), dst2 = alloc_limbs(need + 2),
                       dst3 = alloc_limbs(need + 2);
                mp_size_t rn = lmmp_pow_(dst, need, base, n, exp);
                mp_size_t r2 = lmmp_pow_win2_(dst2, need, base, n, exp);
                TEST_CHECK_MSG(rn == r2 && memcmp(dst, dst2, (size_t)rn * 8) == 0,
                               "pow_ vs pow_win2_");
                // 位长区间：2^(bits-1) <= b < 2^bits ⟹ b^e ∈ [2^(e(bits-1)), 2^(e*bits))
                u64 bits = (u64)(n - 1) * 64 + (u64)top_bits(base[n - 1]);
                mp_size_t lo = (mp_size_t)(((bits - 1) * exp - 1) / 64) + 1;
                mp_size_t hi = (mp_size_t)((bits * exp - 1) / 64) + 1;
                TEST_CHECK_MSG(rn >= lo && rn <= hi && (rn == 0 || dst[rn - 1] != 0),
                               "pow length bounds");
                // 模检查（dst 为全值 b^e，模 p 比对合法）
                bool ok = true;
                for (int i = 0; i < MOD_P_CNT && ok; ++i) {
                    u32 p = MOD_P[i];
                    ok = mod32(dst, rn, p) == modpow32(mod32(base, n, p), exp, p);
                }
                TEST_CHECK_MSG(ok, "pow modpow check");
                // 小指数乘法链重建（b^1/b^2/b^3 经库乘精确比对；乒乓双缓冲
                // 保持 sep(dst,a,b)）
                if (exp <= 3) {
                    mp_ptr c1 = alloc_limbs(need + 2), c2 = alloc_limbs(need + 2);
                    lmmp_copy(c1, base, n);
                    mp_size_t cn = n;
                    for (ulong k = 1; k < exp; ++k) {
                        lmmp_mul_(c2, c1, cn, base, n);  // cn >= n 恒成立
                        mp_ptr tswap = c1; c1 = c2; c2 = tswap;
                        cn += n;
                    }
                    TEST_CHECK_MSG(mpn_eq(dst, rn, c1, cn), "pow chain rebuild");
                    lmmp_free(c1);
                    lmmp_free(c2);
                }
                // pow_basecase_（奇 exp 3..17，旧界内）
                if (exp % 2 == 1 && exp >= 3 && exp <= 17) {
                    mp_size_t r3 = lmmp_pow_basecase_(dst3, need, base, n, exp);
                    TEST_CHECK_MSG(r3 == rn && memcmp(dst, dst3, (size_t)rn * 8) == 0,
                                   "pow_ vs pow_basecase_");
                }
                lmmp_free(dst);
                lmmp_free(dst2);
                lmmp_free(dst3);
            }
            lmmp_free(base);
        }
    }
}

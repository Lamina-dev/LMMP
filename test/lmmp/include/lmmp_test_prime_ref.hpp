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

#ifndef LMMP_TEST_PRIME_REF_HPP
#define LMMP_TEST_PRIME_REF_HPP

/*
    素性检验测试共享构造（铁律合规：仅 64 位内标量参考 + 库路径组合）。
    ref_is_prime64 为原生标量 Miller-Rabin（2^64 确定性基组，模乘经
    lmmp_mullh_ 全积 + lmmp_mod_1_ 取余——均为独立测试套覆盖的原语），
    属「64 位内简单参考」允许域。Proth 证书走 lmmp_powmod_ 独立梯子
    （由 powmod 测试套独立保证），普罗斯定理：有证书 ⟺ 素数。
*/

#include <initializer_list>

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
#include "lmmp_test_utils.hpp"

namespace lmmp_test_prime_ref {

using namespace lmmp_test_utils;

// 64 位模乘：mullh 全积 + mod_1 余（div 套件独立覆盖）
inline u64 mulmod64(u64 a, u64 b, u64 m) {
    mp_limb_t t[2];
    lmmp_mullh_(a, b, t);
    return lmmp_mod_1_(t, 2, m);
}

inline u64 mod_pow64(u64 a, u64 e, u64 mod) {
    u64 r = 1 % mod;
    a %= mod;
    while (e) {
        if (e & 1) r = mulmod64(r, a, mod);
        a = mulmod64(a, a, mod);
        e >>= 1;
    }
    return r;
}

inline bool ref_is_prime64(u64 n) {
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
            x = mulmod64(x, x, n);
            if (x == n - 1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

inline bool ref_is_prime32(u32 n) { return ref_is_prime64(n); }

// 三态期望值：素数时按 SWbound 分界返回 1（确定性）或 2（BPSW 型）
inline bool less_sw(u64 lo, u64 hi) {
    return hi < 0x2be69ull || (hi == 0x2be69ull && lo < 0x51adc5b22410a5fdull);
}
inline int expect2(u64 lo, u64 hi, bool prime) {
    if (!prime) return 0;
    return less_sw(lo, hi) ? 1 : 2;
}

/* 已证 Mersenne 素数指数（GIMPS）与 Mersenne 合数指数（素数 p 且
   p ∈ (127,521)——该区间无 Mersenne 素数 ⟹ M_p 全为合数） */
constexpr int MP_EXP[] = {521, 607, 1279, 4423};
constexpr int MP_FOLD_EXP = 19937; /* 312 limb >= REDC_MERSENNE_THRESHOLD */
constexpr int MC_EXP[] = {131, 137, 149, 163}; /* 全部合数；M131 最小因子 263 */

/* 2^p - 1 写入 dst（p 为奇素数，p%64 != 0），返回 limb 数 */
inline mp_size_t mersenne_limbs(mp_ptr dst, int p) {
    mp_size_t n = p / 64 + 1;
    for (mp_size_t i = 0; i < n; i++) dst[i] = ~(mp_limb_t)0;
    dst[n - 1] = ((mp_limb_t)1 << (p % 64)) - 1;
    return n;
}

/* k*2^t + c（k <= 2^64，c ∈ {0,1}）写入 dst，返回规范化 limb 数 */
inline mp_size_t k2t_limbs(mp_ptr dst, int t, mp_limb_t k, mp_limb_t c) {
    mp_size_t n = (mp_size_t)t / 64 + 2;
    lmmp_zero(dst, n + 1);
    dst[t / 64] |= k << (t % 64);
    if (t % 64) dst[t / 64 + 1] |= k >> (64 - t % 64);
    if (c) dst[0] |= 1;
    while (n > 1 && dst[n - 1] == 0) n--;
    return n;
}

/* 随机 u64 素数（ref_is_prime64 确定性认证），顶位不低于 2^59（保证
   三素子积 > 2^177 达 3 limb） */
inline u64 random_u64_prime(u64& seed) {
    u64 x = (xorshift64(seed) >> 4) | ((u64)1 << 59) | 1;
    while (!ref_is_prime64(x)) x += 2;
    return x;
}

/* 自顶去零（乘积的顶 limb 可能为零，is_prime_n_/lucas 契约要求非零） */
inline mp_size_t limbs_norm(const mp_limb_t* p, mp_size_t n) {
    while (n > 1 && p[n - 1] == 0) n--;
    return n;
}

/*
    Proth 证书（通用 k，t <= 20000）：N = k*2^t+1 为素 ⟺ 存在小奇 a 使
    a^((N-1)/2) ≡ -1 (mod N)；走 lmmp_powmod_ 通用路径（独立于被测实现）。
    nbase 为基底表用量（默认全表至 61），求不出仅返回 false
*/
inline bool proth_cert_g(uint k, int t, int nbase = 17) {
    static const u64 bases[] = {3,  5,  7,  11, 13, 17, 19, 23, 29, 31,
                                37, 41, 43, 47, 53, 59, 61};
    mp_limb_t N[512], e[512], bp[512], r[512], m1[512];
    mp_size_t nn = k2t_limbs(N, t, k, 1);
    mp_size_t en = k2t_limbs(e, t - 1, k, 0);
    lmmp_copy(m1, N, nn);
    lmmp_dec(m1);
    for (int ai = 0; ai < nbase; ai++) {
        lmmp_zero(bp, nn);
        bp[0] = bases[ai];
        lmmp_powmod_(r, bp, e, en, N, nn);
        if (lmmp_cmp_(r, m1, nn) == 0) return true;
    }
    return false;
}

/*
    Proth 证书（k=3 定长形式，n = 3*2^t+1）：被测命中即证书复核
*/
inline bool proth_certified(const mp_limb_t* np, mp_size_t nn, int t) {
    mp_limb_t e[8], bp[512], r[512], m1[512];
    mp_size_t en = k2t_limbs(e, t - 1, 3, 0); /* (n-1)/2 = 3*2^(t-1) */
    lmmp_copy(m1, np, nn);
    lmmp_dec(m1);
    for (u64 a : {3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61}) {
        lmmp_zero(bp, nn);
        bp[0] = a;
        lmmp_powmod_(r, bp, e, en, np, nn);
        if (lmmp_cmp_(r, m1, nn) == 0) return true;
    }
    return false;
}

}  // namespace lmmp_test_prime_ref

#endif  // LMMP_TEST_PRIME_REF_HPP

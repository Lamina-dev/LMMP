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

/* 模幂家族验证（无 BigInt oracle；整数幂见 test_numth_pow.cpp）：
   1. 费马小定理（素模 p：b^(p-1)==1）、指数律、乘法性、
      n==1 时 u64 简单参考（mullh 乘 + mod_1 余）、奇模数下
      powmod_/powmod_1_/powmod_2_/powmod_odd_ 四入口一致、偶模数
      CRT 代数重建、(-1)^e 与 b=0/e=1 写死角点
   2. powlo 与 powmod_(B^n) 双算法互比（值恒相等）
   3. REDC / mont_sp_redc：值域界 + r*B^n ≡ t (mod m) 同余重建
*/

#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"
/* impl/powmod.h 为 C 头（restrict 限定符），C++ TU 以 __restrict 适配后包含 */
#define restrict __restrict
#include "lmmp/impl/powmod.h"
#undef restrict
#include "lmmp_test.hpp"
#include "lmmp_test_utils.hpp"

#include <cstring>

using namespace lmmp_test_utils;

namespace {

// 64 位内任意模数参考模幂：mullh 全积 + mod_1 取余（独立于被测蒙域路径）
u64 powmod_ref_u64(u64 b, const u64* ep, mp_size_t en, u64 m) {
    u64 r = 1 % m;
    b %= m;
    mp_limb_t t[2];
    for (mp_size_t i = en; i-- > 0;) {
        for (int bit = 63; bit >= 0; --bit) {
            lmmp_mullh_(r, r, t);
            r = lmmp_mod_1_(t, 2, m);
            if ((ep[i] >> bit) & 1) {
                lmmp_mullh_(r, b, t);
                r = lmmp_mod_1_(t, 2, m);
            }
        }
    }
    return r;
}

// 顶部 limb 有效位数
int top_bits(u64 x) { return 64 - __builtin_clzll(x); }

// 奇模数构造：m 随机奇归一化；b 归约到 < m（div_ 一步）
void make_odd_mod(mp_ptr m, mp_ptr b, mp_size_t n, u64& seed) {
    random_limbs(m, n, seed);
    m[0] |= 1;
    mp_ptr raw = alloc_limbs(n), q = alloc_limbs(n);
    random_limbs(raw, n, seed);
    raw[n - 1] >>= 1;  // raw < 2^(64n-1)，一步归约后必 < m
    if (raw[n - 1] == 0) raw[n - 1] = 1;
    lmmp_div_(q, b, raw, n, m, n);
    lmmp_free(raw);
    lmmp_free(q);
}

// 模乘归约：[a,n]*[b,n] mod [m,n] → r（库乘 + 库除）
void mulmod_limbs(mp_ptr r, const mp_limb_t* a, const mp_limb_t* b, const mp_limb_t* m, mp_size_t n,
                  mp_ptr t, mp_ptr q) {
    lmmp_mul_(t, a, n, b, n);
    lmmp_div_(q, r, t, 2 * n, m, n);
}

// m = k*2^e + form（k <= 2^64，e >= 64）装配到 dst（容量 e/64+3），返回规范化长度
mp_size_t sp_shape_limbs(mp_ptr dst, u64 k, int e, int form) {
    size_t cap = (size_t)e / 64 + 3;
    lmmp_zero(dst, cap);
    int s = e % 64;
    dst[e / 64] = s ? (k << s) : k;
    if (s) dst[e / 64 + 1] = k >> (64 - s);
    if (form > 0) dst[0] += 1;
    else lmmp_dec(dst);
    return norm_len(dst, (mp_size_t)cap);
}

}  // namespace

TEST_CASE("numth/powmod", redc_value) {
    u64 seed = 0x9e3779b97f4a7c15ull;
    // 覆盖 REDC 两条路径：小尺寸全积取高半、>= REDC_MERSENNE_THRESHOLD 的梅森折叠
    for (mp_size_t n : {1, 2, 3, 5, 9, 17, 33, 50, 402}) {
        for (int iter = 0; iter < 3; ++iter) {
            mp_ptr m = alloc_limbs(n), a = alloc_limbs(n), b = alloc_limbs(n);
            make_odd_mod(m, a, n, seed);
            make_odd_mod(m, b, n, seed);
            mp_ptr t = alloc_limbs(2 * n), dst = alloc_limbs(n + 1), ninv = alloc_limbs(n);
            lmmp_mul_(t, a, n, b, n);
            lmmp_binvert_(ninv, m, n, n);
            // ninv = -m^(-1) mod B^n（两补码取负；逆元为奇不会越顶）
            u64 carry = 1;
            for (mp_size_t i = 0; i < n; ++i) {
                u64 x = ~ninv[i];
                u64 s = x + carry;
                carry = (s == 0) ? 1u : 0u;
                ninv[i] = s;
            }
            mp_limb_t cy = lmmp_redc_(dst, t, ninv, m, n);
            TEST_CHECK_MSG(cy <= 1, "redc carry range");
            // u = dst + cy*B^n；u < 2m
            mp_ptr u = alloc_limbs(n + 1);
            lmmp_zero(u, n + 1);
            memcpy(u, dst, (size_t)n * 8);
            if (cy) u[n] = 1;
            // 2m = m+m（库加）
            mp_ptr twom2 = alloc_limbs(n + 1);
            lmmp_zero(twom2, n + 1);
            lmmp_copy(twom2, m, n);
            lmmp_add_n_(twom2, twom2, twom2, n + 1);
            TEST_CHECK_MSG(mpn_cmp(u, n + 1, twom2, n + 1) < 0, "redc < 2m");
            // u*B^n ≡ t (mod m)：同余仅对模 m 成立，经库除法（Knuth D，独立
            // 路径）双侧取余精确比对：(u*B^n) mod m == (a*b) mod m
            {
                mp_ptr P = alloc_limbs(2 * n + 2), q2 = alloc_limbs(n + 2), rm = alloc_limbs(n),
                       rt = alloc_limbs(n);
                lmmp_zero(P, 2 * n + 2);
                memcpy(P + n, u, (size_t)(n + 1) * 8);  // P = u*B^n（≤ 2n+1 limbs）
                lmmp_div_(q2, rm, P, 2 * n + 1, m, n);
                lmmp_div_(q2, rt, t, 2 * n, m, n);
                TEST_CHECK_MSG(mpn_eq(rm, n, rt, n), "redc (u*B^n) mod m == t mod m");
                lmmp_free(P); lmmp_free(q2); lmmp_free(rm); lmmp_free(rt);
            }
            lmmp_free(m); lmmp_free(a); lmmp_free(b); lmmp_free(t); lmmp_free(dst);
            lmmp_free(ninv); lmmp_free(twom2); lmmp_free(u);
        }
    }
}

TEST_CASE("numth/powmod", powmod_odd_and_dispatch) {
    u64 seed = 0x1234567890abcdefull;
    for (mp_size_t n : {1, 2, 3, 4, 7, 12, 33, 70}) {
        for (int iter = 0; iter < 3; ++iter) {
            mp_ptr m = alloc_limbs(n), b = alloc_limbs(n);
            make_odd_mod(m, b, n, seed);
            mp_ptr dst = alloc_limbs(n), dst2 = alloc_limbs(n), dst3 = alloc_limbs(n);
            // 指数集：短/边界/单双三 limb
            struct { mp_size_t en; u64 w[3]; } exps[] = {
                {1, {1, 0, 0}},          {1, {2, 0, 0}},          {1, {3, 0, 0}},
                {1, {17, 0, 0}},         {1, {64, 0, 0}},         {1, {0xdeadbeefull, 0, 0}},
                {1, {0xffffffffffffffffull, 0, 0}},
                {2, {0x9e3779b97f4a7c15ull, 0x1234, 0}},
                {3, {0, 0, 1}},  // e=B^2：偶底数短路形态
            };
            for (const auto& ex : exps) {
                mp_ptr ep = alloc_limbs(ex.en);
                memcpy(ep, ex.w, (size_t)ex.en * 8);
                lmmp_powmod_odd_(dst, b, ep, ex.en, m, n);
                TEST_CHECK_MSG(mpn_cmp(dst, n, m, n) < 0, "powmod_odd in range");
                // n==1：u64 简单参考（mullh+mod_1，指数全 limb）
                if (n == 1) {
                    u64 want = powmod_ref_u64(b[0], ep, ex.en, m[0]);
                    TEST_CHECK_MSG(dst[0] == want, "powmod_odd vs u64 reference");
                }
                // 入口一致：powmod_ 分派（奇模数走 odd 路径）
                lmmp_powmod_(dst2, b, ep, ex.en, m, n);
                TEST_CHECK_MSG(memcmp(dst, dst2, (size_t)n * 8) == 0, "powmod_ dispatch == odd");
                // n==1/2 特化入口一致
                if (n == 1) {
                    lmmp_powmod_1_(dst3, b, ep, ex.en, m[0]);
                    TEST_CHECK_MSG(dst[0] == dst3[0], "powmod_1_ == odd");
                }
                if (n == 2) {
                    lmmp_powmod_2_(dst3, b, ep, ex.en, m);
                    TEST_CHECK_MSG(memcmp(dst, dst3, 16) == 0, "powmod_2_ == odd");
                }
                lmmp_free(ep);
            }
            lmmp_free(m); lmmp_free(b); lmmp_free(dst); lmmp_free(dst2); lmmp_free(dst3);
        }
    }

    // 费马小定理：素模 p ∈ MOD_P，b^(p-1) == 1；b^p == b（固定 b）
    for (int i = 0; i < MOD_P_CNT; ++i) {
        u32 p = MOD_P[i];
        for (u64 bb : {2ull, 3ull, 12345ull, (u64)p - 1}) {
            u64 b = bb % p;
            if (b == 0) continue;
            mp_limb_t mp[1] = {p}, bp[1] = {b}, ep[1] = {p - 1}, dst[1];
            lmmp_powmod_odd_(dst, bp, ep, 1, mp, 1);
            TEST_CHECK_MSG(dst[0] == 1, "fermat b^(p-1) == 1");
            ep[0] = p;
            lmmp_powmod_odd_(dst, bp, ep, 1, mp, 1);
            TEST_CHECK_MSG(dst[0] == b, "fermat b^p == b");
        }
    }

    // 角点：m = B^n-1（全 1 折叠拼接二义点）、b=0、b=m-1、e=1
    for (mp_size_t n : {2, 5, 17}) {
        mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), dst = alloc_limbs(n), ep = alloc_limbs(2);
        for (mp_size_t i = 0; i < n; ++i) m[i] = ~(u64)0;
        ep[0] = 0x24f1a7e3c5590b8dull;
        ep[1] = 1;
        lmmp_zero(b, n);
        lmmp_powmod_odd_(dst, b, ep, 2, m, n);
        bool zero_ok = true;
        for (mp_size_t i = 0; i < n; ++i) zero_ok = zero_ok && dst[i] == 0;
        TEST_CHECK_MSG(zero_ok, "powmod b=0");
        // b = m-1：(-1)^e，e 奇（ep 顶 limb=1 ⟹ 奇）→ m-1；e=2 → 1
        for (mp_size_t i = 0; i < n; ++i) b[i] = ~(u64)0;
        b[0] -= 1;
        lmmp_powmod_odd_(dst, b, ep, 2, m, n);
        bool m1_ok = true;
        for (mp_size_t i = 0; i < n; ++i) m1_ok = m1_ok && dst[i] == b[i];
        TEST_CHECK_MSG(m1_ok, "powmod (-1)^odd == m-1");
        ep[0] = 2;
        lmmp_powmod_odd_(dst, b, ep, 1, m, n);
        bool one_ok = dst[0] == 1;
        for (mp_size_t i = 1; i < n; ++i) one_ok = one_ok && dst[i] == 0;
        TEST_CHECK_MSG(one_ok, "powmod (-1)^2 == 1");
        // e=1 拷贝快路径
        u64 s3 = seed ^ (u64)n;
        random_limbs(b, n, s3);
        lmmp_div_(NULL, b, b, n, m, n);  // b mod m（原地余数）
        ep[0] = 1;
        lmmp_powmod_odd_(dst, b, ep, 1, m, n);
        TEST_CHECK_MSG(memcmp(dst, b, (size_t)n * 8) == 0, "powmod e=1 == b");
        lmmp_free(m); lmmp_free(b); lmmp_free(dst); lmmp_free(ep);
    }

    // 梅森折叠主路径（n >= REDC_MERSENNE_THRESHOLD）：指数律（积经库乘+库除归约）
    for (mp_size_t n : {402, 405}) {
        mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), dst = alloc_limbs(n), r1 = alloc_limbs(n),
               r2 = alloc_limbs(n), r12 = alloc_limbs(n), t = alloc_limbs(2 * n), q = alloc_limbs(2 * n);
        make_odd_mod(m, b, n, seed);
        u64 e1 = 0x9e3779b9ull >> 1, e2 = 0x9e3779b9ull - e1;
        mp_limb_t ep1[1] = {e1}, ep2[1] = {e2}, ep12[1] = {e1 + e2};
        lmmp_powmod_odd_(r1, b, ep1, 1, m, n);
        lmmp_powmod_odd_(r2, b, ep2, 1, m, n);
        lmmp_powmod_odd_(dst, b, ep12, 1, m, n);
        TEST_CHECK_MSG(mpn_cmp(dst, n, m, n) < 0, "fold result in range");
        lmmp_mul_(t, r1, n, r2, n);
        lmmp_div_(q, r12, t, 2 * n, m, n);
        TEST_CHECK_MSG(memcmp(dst, r12, (size_t)n * 8) == 0, "fold exponent law");
        lmmp_free(m); lmmp_free(b); lmmp_free(dst); lmmp_free(r1); lmmp_free(r2);
        lmmp_free(r12); lmmp_free(t); lmmp_free(q);
    }
}

TEST_CASE("numth/powmod", powmod_exponent_and_product_laws) {
    // 指数律 b^(e1+e2) == b^e1 * b^e2 (mod m) 与乘法性 (a*c)^e == a^e * c^e (mod m)
    u64 seed = 0x13198a2e03707344ull;
    for (mp_size_t n : {1, 2, 3, 5, 17, 33, 70}) {
        for (int iter = 0; iter < 4; ++iter) {
            mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), c = alloc_limbs(n);
            make_odd_mod(m, b, n, seed);
            make_odd_mod(m, c, n, seed);
            mp_ptr t = alloc_limbs(2 * n + 2), q = alloc_limbs(2 * n + 2), r1 = alloc_limbs(n),
                   r2 = alloc_limbs(n), r12 = alloc_limbs(n), dst = alloc_limbs(n);
            // 指数律：e1+e2（64 位内），积经库乘 + 库除归约
            u64 e1 = xorshift64(seed) >> (1 + iter % 32), e2 = xorshift64(seed) >> (1 + iter % 16);
            mp_limb_t ep1[1] = {e1}, ep2[1] = {e2}, ep12[1] = {e1 + e2};
            lmmp_powmod_odd_(r1, b, ep1, 1, m, n);
            lmmp_powmod_odd_(r2, b, ep2, 1, m, n);
            lmmp_powmod_odd_(dst, b, ep12, 1, m, n);
            mulmod_limbs(r12, r1, r2, m, n, t, q);
            TEST_CHECK_MSG(memcmp(dst, r12, (size_t)n * 8) == 0, "b^(e1+e2) == b^e1*b^e2");
            // 乘法性：(b*c mod m)^e == b^e * c^e (mod m)
            u64 e = 0x9e3779b97f4a7c15ull;
            mp_limb_t ep[1] = {e};
            mp_ptr bc = alloc_limbs(n);
            mulmod_limbs(bc, b, c, m, n, t, q);
            lmmp_powmod_odd_(r1, bc, ep, 1, m, n);
            lmmp_powmod_odd_(r2, b, ep, 1, m, n);
            lmmp_powmod_odd_(dst, c, ep, 1, m, n);
            mulmod_limbs(r12, r2, dst, m, n, t, q);
            TEST_CHECK_MSG(memcmp(r1, r12, (size_t)n * 8) == 0, "(bc)^e == b^e*c^e");
            lmmp_free(m); lmmp_free(b); lmmp_free(c); lmmp_free(bc);
            lmmp_free(t); lmmp_free(q); lmmp_free(r1); lmmp_free(r2); lmmp_free(r12);
            lmmp_free(dst);
        }
    }
}

TEST_CASE("numth/powmod", powlo_family) {
    u64 seed = 0x1b873593f00dbeebull;
    // powlo vs powmod_(B^n)：b^e mod B^n 的两条独立算法（截断梯子 vs 偶模数 CRT）
    for (mp_size_t n : {1, 2, 3, 5, 17}) {
        for (int iter = 0; iter < 3; ++iter) {
            mp_ptr b = alloc_limbs(n), ep = alloc_limbs(3), dst = alloc_limbs(n),
                   dst2 = alloc_limbs(n + 1), bn = alloc_limbs(n + 1), bp = alloc_limbs(n + 1);
            random_limbs(b, n, seed);
            lmmp_zero(bn, n + 1);
            bn[n] = 1;  // 模数 B^n（n+1 limb，顶 limb=1）
            for (mp_size_t i = 0; i < 3; ++i) ep[i] = xorshift64(seed);
            ep[2] |= (u64)1 << 63;
            lmmp_zero(bp, n + 1);
            memcpy(bp, b, (size_t)n * 8);  // b < B^n
            for (ulong e : {1ull, 2ull, 5ull, 0xdeadbeefull, ep[0]}) {
                mp_limb_t e1[1] = {e};
                lmmp_powlo_(dst, b, n, e1, 1);
                lmmp_powmod_(dst2, bp, e1, 1, bn, n + 1);
                TEST_CHECK_MSG(memcmp(dst, dst2, (size_t)n * 8) == 0, "powlo == powmod(B^n)");
            }
            lmmp_powlo_(dst, b, n, ep, 3);
            lmmp_powmod_(dst2, bp, ep, 3, bn, n + 1);
            TEST_CHECK_MSG(memcmp(dst, dst2, (size_t)n * 8) == 0, "powlo 3-limb exp == powmod");
            // 特化与分派一致
            if (n == 1) {
                mp_ptr d1 = alloc_limbs(1);
                lmmp_powlo_1_(d1, b, ep, 3);
                TEST_CHECK_MSG(d1[0] == dst[0], "powlo_1_ == powlo_");
                lmmp_free(d1);
            }
            if (n == 2) {
                mp_ptr d2 = alloc_limbs(2);
                lmmp_powlo_2_(d2, b, ep, 3);
                TEST_CHECK_MSG(memcmp(d2, dst, 16) == 0, "powlo_2_ == powlo_");
                lmmp_free(d2);
            }
            // 截断恒等式：powlo(b,e1+e2) == low_n(powlo(b,e1)*powlo(b,e2))
            u64 e1 = ep[0] >> 1, e2 = ep[0] - e1;
            mp_limb_t ea[1] = {e1}, eb[1] = {e2};
            mp_ptr p1 = alloc_limbs(n), p2 = alloc_limbs(n), prod = alloc_limbs(2 * n), dm = alloc_limbs(n);
            lmmp_powlo_(p1, b, n, ea, 1);
            lmmp_powlo_(p2, b, n, eb, 1);
            lmmp_powlo_(dm, b, n, ep, 1);
            lmmp_mul_(prod, p1, n, p2, n);
            TEST_CHECK_MSG(memcmp(dm, prod, (size_t)n * 8) == 0, "powlo truncation law");
            // e=1 快路径 == b
            mp_limb_t e1limb[1] = {1};
            lmmp_powlo_(dm, b, n, e1limb, 1);
            TEST_CHECK_MSG(memcmp(dm, b, (size_t)n * 8) == 0, "powlo e=1 == b");
            lmmp_free(p1); lmmp_free(p2); lmmp_free(prod); lmmp_free(dm);
            lmmp_free(b); lmmp_free(ep); lmmp_free(dst); lmmp_free(dst2); lmmp_free(bn);
            lmmp_free(bp);
        }
    }
}

TEST_CASE("numth/powmod", powmod_even_two_adic) {
    // 偶模数 m = odd << k：(-1)^e / e=1 / b=0 角点 + 指数律 + 奇分派一致 + CRT 重建
    u64 seed = 0x243f6a8885a308d3ull;
    const size_t cap = 12;
    size_t ks[] = {1, 2, 63, 64, 65, 128, 129, 384, 511, 512};
    for (size_t k : ks) {
        u64 odds[3] = {1, 3, xorshift64(seed) | 1};
        for (u64 odd : odds) {
            // m = odd << k：跨 limb 由 limb 定位承载，limb 内由 shl 位移承载
            mp_size_t nb = k / 64 + 2;
            if (nb > cap) continue;
            mp_ptr m = alloc_limbs(cap), b = alloc_limbs(cap), dst = alloc_limbs(cap);
            lmmp_zero(m, cap);
            int kb = (int)(k % 64);
            m[k / 64] = kb ? (odd << kb) : odd;
            if (kb) m[k / 64 + 1] = odd >> (64 - kb);
            mp_size_t n = norm_len(m, cap);
            // 归约基 b < m：随机后除
            u64 s5 = seed ^ (u64)k * 0x9e3779b97f4a7c15ull ^ odd;
            random_limbs(b, n, s5);
            b[n - 1] >>= 1;
            if (b[n - 1] == 0) b[n - 1] = 1;
            mp_ptr q = alloc_limbs(cap);
            lmmp_div_(q, b, b, n, m, n);
            lmmp_free(q);
            // b=0 → 0；e=1 → b；b=m-1：奇 e → m-1、偶 e → 1
            mp_ptr z = alloc_limbs(cap);
            lmmp_zero(z, n);
            mp_limb_t e2[1] = {2}, eodd[1] = {0x51ed270b9a3c8f11ull}, e1[1] = {1};
            lmmp_powmod_(dst, z, eodd, 1, m, n);
            bool zk = norm_len(dst, n) == 0;
            TEST_CHECK_MSG(zk, "even-mod b=0");
            lmmp_powmod_(dst, b, e1, 1, m, n);
            TEST_CHECK_MSG(memcmp(dst, b, (size_t)n * 8) == 0, "even-mod e=1 == b");
            mp_ptr mm1 = alloc_limbs(cap);
            lmmp_copy(mm1, m, n);
            sub_small_inplace(mm1, n, 1);
            lmmp_powmod_(dst, mm1, eodd, 1, m, n);
            TEST_CHECK_MSG(memcmp(dst, mm1, (size_t)n * 8) == 0, "(-1)^odd == m-1");
            lmmp_powmod_(dst, mm1, e2, 1, m, n);
            TEST_CHECK_MSG(dst[0] == 1 && norm_len(dst, n) == 1, "(-1)^2 == 1");
            // 指数律（b^e1 * b^e2 == b^(e1+e2)）
            {
                u64 ea = eodd[0] >> 3, eb = eodd[0] - ea;
                mp_limb_t epa[1] = {ea}, epb[1] = {eb}, eps[1] = {eodd[0]};
                mp_ptr r1 = alloc_limbs(cap), r2 = alloc_limbs(cap), t = alloc_limbs(2 * cap),
                       qq = alloc_limbs(2 * cap);
                lmmp_powmod_(r1, b, epa, 1, m, n);
                lmmp_powmod_(r2, b, epb, 1, m, n);
                lmmp_powmod_(dst, b, eps, 1, m, n);
                lmmp_mul_(t, r1, n, r2, n);
                lmmp_div_(qq, r1, t, 2 * n, m, n);
                TEST_CHECK_MSG(memcmp(dst, r1, (size_t)n * 8) == 0, "even-mod exponent law");
                lmmp_free(r1); lmmp_free(r2); lmmp_free(t); lmmp_free(qq);
            }
            // CRT 同余组验证：dst mod 2^k == b^e mod 2^k 且 dst mod odd == b^e mod odd
            // （odd==1 时退化为纯 2^k 模数，跳过）
            if (odd > 1) {
                mp_limb_t odb[1] = {odd};
                // r2 = b^e mod odd
                mp_ptr bmod = alloc_limbs(cap), r2 = alloc_limbs(cap);
                lmmp_div_(NULL, bmod, b, n, odb, 1);
                lmmp_powmod_(r2, bmod, eodd, 1, odb, 1);
                lmmp_powmod_(dst, b, eodd, 1, m, n);
                mp_ptr p2k = alloc_limbs(cap), b2 = alloc_limbs(cap), r1 = alloc_limbs(cap),
                       d2 = alloc_limbs(cap);
                lmmp_zero(p2k, cap);
                p2k[k / 64] |= (u64)1 << (k % 64);
                mp_size_t np = norm_len(p2k, cap);
                lmmp_div_(NULL, b2, b, n, p2k, np);
                lmmp_powmod_(r1, b2, eodd, 1, p2k, np);
                lmmp_div_(NULL, d2, dst, n, p2k, np);
                TEST_CHECK_MSG(mpn_eq(d2, np, r1, np), "CRT: result mod 2^k");
                lmmp_div_(NULL, d2, dst, n, odb, 1);
                TEST_CHECK_MSG(d2[0] == r2[0], "CRT: result mod odd");
                lmmp_free(p2k); lmmp_free(b2); lmmp_free(r1); lmmp_free(d2);
                lmmp_free(bmod); lmmp_free(r2);
            }
            lmmp_free(m); lmmp_free(b); lmmp_free(dst); lmmp_free(z); lmmp_free(mm1);
        }
    }
    // 奇模数经 powmod_ 分派与 powmod_odd_ 同值
    {
        u64 s2 = 0xa4093822299f31d0ull;
        for (mp_size_t n : {1, 5, 17}) {
            mp_ptr m = alloc_limbs(n), b = alloc_limbs(n), d1 = alloc_limbs(n), d2 = alloc_limbs(n);
            make_odd_mod(m, b, n, s2);
            mp_limb_t ep[1] = {0x9e3779b9ull};
            lmmp_powmod_(d1, b, ep, 1, m, n);
            lmmp_powmod_odd_(d2, b, ep, 1, m, n);
            TEST_CHECK_MSG(memcmp(d1, d2, (size_t)n * 8) == 0, "odd dispatch match");
            lmmp_free(m); lmmp_free(b); lmmp_free(d1); lmmp_free(d2);
        }
    }
}

/*
    稀疏模数蒙域旁路 lmmp_mont_sp_redc_ 直测（含探测）：
    - ±1 两形 × 簇偏移跨 limb 边界 × 尺寸跨 REDC 阈值；
    - 值域 r < m 与同余 r·B^n ≡ t (mod m) 经库除法双侧取余精确比对；
    - 探测否定侧：稠密、全 B−1、3 limb 簇均返回 0。
*/
TEST_CASE("numth/powmod", mont_sp_redc_value) {
    u64 seed = 0x5ec0fefe1e55aa21ull;
    struct { u64 k; int e; int form; } shapes[] = {
        {1, 64, 1}, {3, 64, 1}, {3, 65, 1}, {0xffffffffull, 127, 1},
        {0xdeadbeefcafebull, 127, 1}, {0x123456789abcull, 129, 1},
        {3, 300, 1}, {1, 1024, 1}, {3, 19969, 1},
        {1, 127, -1}, {3, 64, -1}, {0xdeadbeefcafebull, 127, -1},
        {0x123456789abcull, 191, -1}, {3, 300, -1}, {1, 19937, -1},
    };
    for (const auto& sh : shapes) {
        size_t cap = (size_t)sh.e / 64 + 3;
        mp_ptr m = alloc_limbs(cap), r = alloc_limbs(cap), t = alloc_limbs(2 * cap);
        mp_size_t n = sp_shape_limbs(m, sh.k, sh.e, sh.form);

        lmmp_mont_sp_t mc;
        int form = lmmp_mont_sp_init_(&mc, m, n);
        TEST_CHECK_MSG(form == sh.form, "sp probe form");

        int samples = n > 100 ? 2 : 4;
        for (int it = 0; it < samples; ++it) {
            random_limbs(t, 2 * (size_t)n, seed);
            /* 顶半压制到 < m：顶 limb 掩码到 m 位长内，仍越则整体减 m */
            u64 tb = (u64)top_bits(m[n - 1]);
            t[2 * (size_t)n - 1] &= (tb == 64) ? ~(u64)0 : (((u64)1 << tb) - 1);
            if (lmmp_cmp_(t + n, m, n) >= 0) (void)lmmp_sub_n_(t + n, t + n, m, n);

            lmmp_mont_sp_redc_(r, t, &mc);
            TEST_CHECK_MSG(lmmp_cmp_(r, m, n) < 0, "sp redc < m");
            // r*B^n ≡ t (mod m)：经库除法双侧取余精确比对（t < m*B^n 已构造）
            {
                mp_ptr P = alloc_limbs(2 * n), q = alloc_limbs(n + 2), rm = alloc_limbs(n),
                       rt = alloc_limbs(n);
                lmmp_zero(P, 2 * n);
                memcpy(P + n, r, (size_t)n * 8);  // P = r*B^n
                lmmp_div_(q, rm, P, 2 * n, m, n);
                lmmp_div_(q, rt, t, 2 * n, m, n);
                TEST_CHECK_MSG(mpn_eq(rm, n, rt, n), "sp redc congruence");
                lmmp_free(P); lmmp_free(q); lmmp_free(rm); lmmp_free(rt);
            }
        }
        lmmp_free(m); lmmp_free(r); lmmp_free(t);
    }

    /* 探测否定侧 */
    lmmp_mont_sp_t mc;
    {
        mp_ptr m = alloc_limbs(4);
        mp_size_t n = sp_shape_limbs(m, 3, 100, 1);
        m[0] += 2; /* 稠密：m[0] = 3，中间零带被 +2 破坏以外形也不再 ±1 */
        TEST_CHECK_MSG(lmmp_mont_sp_init_(&mc, m, n) == 0, "sp probe dense");
        lmmp_free(m);
    }
    {
        mp_ptr m = alloc_limbs(2); /* B^2−1 全 B−1（= B^n−1 整密，无簇） */
        m[0] = ~(u64)0;
        m[1] = ~(u64)0;
        TEST_CHECK_MSG(lmmp_mont_sp_init_(&mc, m, 2) == 0, "sp probe all-ones");
        lmmp_free(m);
    }
    {
        mp_ptr m = alloc_limbs(5); /* 3 limb 簇：m = 1 + K·B^2，K 三 limb */
        lmmp_zero(m, 5);
        m[0] = 1;
        m[2] = 0x13579bdf2468ace0ull;
        m[3] = ~(u64)0;
        m[4] = 1;
        TEST_CHECK_MSG(lmmp_mont_sp_init_(&mc, m, 5) == 0, "sp probe 3-limb cluster");
        lmmp_free(m);
    }
}

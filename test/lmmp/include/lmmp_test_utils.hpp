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

#ifndef LMMP_TEST_UTILS_HPP
#define LMMP_TEST_UTILS_HPP

#include <cstdint>
#include <cstring>

#include "lmmp/lmmp.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

/*
    测试工具集（2026-11-01 重构，2026-11-03 铁律立）：不包含任何自建
    oracle 算法（BigInt/u128 已删除）。铁律：测试禁止引入任何自建算法
    实现作为交叉验证 oracle。验证手段限定为：

      1. 库内交叉验证：两条独立实现路径同输入比对（divexact vs div、
         toom vs fft vs 分派器、mul_1 vs div_1 互逆重建……），且参考
         须与被测算法规模匹配（大规模乘法禁用 basecase 作参考）
      2. 数学恒等式：结果必须满足的代数关系（q*d+r==n、r^2<=n<(r+1)^2、
         fib 递推、gcd 乘法消去律、Cassini……）
      3. 模检查：结果与输入在若干 32 位素数下模值一致（乘积/和/差）
      4. 写死常数：外部已知的固定值锚点（(2^64-1)^2、素数表、
         factorial(10000) 顶 limb 等）
      5. 64 位内的简单参考实现（逐位循环、% 运算等，不引入大整数）

    本头文件只提供 limb 级工具，禁止再引入 128 位类型或大整数结构。
*/

namespace lmmp_test_utils {

using u64 = std::uint64_t;
using u32 = std::uint32_t;

// 简单确定性 xorshift64 随机源，避免测试输入与库 RNG 互相影响。
inline u64 xorshift64(u64& s) {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
}

inline mp_ptr alloc_limbs(size_t n) { return (mp_ptr)lmmp_alloc(n * sizeof(mp_limb_t)); }

// [p,n] = 随机 limbs；top_nonzero 时顶 limb 置最高位（归一化）
inline void random_limbs(mp_ptr p, size_t n, u64& seed, bool top_nonzero = true) {
    for (size_t i = 0; i < n; ++i) p[i] = xorshift64(seed);
    if (n > 0 && top_nonzero) p[n - 1] |= (u64)1 << 63;
}

// 有效长度：去除前导零后的数值长度（n==0 或全零返回 0）
inline mp_size_t norm_len(const mp_limb_t* p, mp_size_t n) {
    while (n > 0 && p[n - 1] == 0) --n;
    return n;
}

// 手工把小值 v 加到 [p,n] 上（v 任意 u64），返回溢出进位
inline u64 add_small_inplace(mp_ptr p, size_t n, u64 v) {
    size_t i = 0;
    u64 old = p[0];
    p[0] += v;
    u64 carry = (p[0] < old) ? 1 : 0;
    while (carry && ++i < n) {
        old = p[i];
        ++p[i];
        carry = (p[i] < old) ? 1 : 0;
    }
    return (carry && i == n) ? 1 : 0;
}

// 手工把小值 v 从 [p,n] 减去，返回借位
inline u64 sub_small_inplace(mp_ptr p, size_t n, u64 v) {
    size_t i = 0;
    u64 old = p[0];
    p[0] -= v;
    u64 borrow = (p[0] > old) ? 1 : 0;
    while (borrow && ++i < n) {
        old = p[i];
        --p[i];
        borrow = (p[i] > old) ? 1 : 0;
    }
    return (borrow && i == n) ? 1 : 0;
}

// [p,n] 右移 1 位，顶位补 top_bit；返回原最低位
inline u64 shr1_inplace(mp_ptr p, size_t n, u64 top_bit) {
    u64 low = p[0] & 1;
    for (size_t i = 0; i + 1 < n; ++i) p[i] = (p[i] >> 1) | (p[i + 1] << 63);
    if (n > 0) p[n - 1] = (p[n - 1] >> 1) | (top_bit << 63);
    return low;
}

// 数值比较（忽略前导零）：a<b 返回 -1，相等 0，a>b 返回 1
inline int mpn_cmp(const mp_limb_t* a, mp_size_t an, const mp_limb_t* b, mp_size_t bn) {
    an = norm_len(a, an);
    bn = norm_len(b, bn);
    if (an != bn) return an < bn ? -1 : 1;
    for (mp_size_t i = an; i-- > 0;)
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
}

inline bool mpn_eq(const mp_limb_t* a, mp_size_t an, const mp_limb_t* b, mp_size_t bn) {
    return mpn_cmp(a, an, b, bn) == 0;
}

/*
    模检查：验证 limb 数组在 32 位素数下的模值。素数均 < 2^32，故两个
    模值的乘积 < 2^64，无需 128 位中间量。被测的大数结果与由输入模值
    直接计算出的期望模值一致，可捕捉绝大多数字节/进位类错误。
*/
constexpr u32 MOD_P[] = {4294967291u, 4294967279u, 4294967231u, 2147483647u, 1000000007u};
constexpr int MOD_P_CNT = (int)(sizeof(MOD_P) / sizeof(MOD_P[0]));

// [p,n] mod m（m 须为奇素数以外亦可用，仅要求 m != 0）；库 mod_1 路径
inline u64 mod32(const mp_limb_t* p, mp_size_t n, u32 m) { return lmmp_mod_1_(p, n, m); }

inline u64 mulmod32(u64 a, u64 b, u64 m) { return (a * b) % m; }  // a,b < m < 2^32
inline u64 addmod32(u64 a, u64 b, u64 m) { return (a + b) % m; }  // a,b < m，和 < 2^33
inline u64 submod32(u64 a, u64 b, u64 m) { return (a + m - b) % m; }

// 模值相等：[a,an] 与 [b,bn] 在全部内置素数下同余
inline bool mod_eq(const mp_limb_t* a, mp_size_t an, const mp_limb_t* b, mp_size_t bn) {
    an = norm_len(a, an);
    bn = norm_len(b, bn);
    for (int i = 0; i < MOD_P_CNT; ++i) {
        u32 p = MOD_P[i];
        if (mod32(a, an, p) != mod32(b, bn, p)) return false;
    }
    return true;
}

// 模乘检查：[r,rn] ≡ [a,an]*[b,bn] (mod p) 对全部内置素数成立
inline bool mod_is_product(const mp_limb_t* r, mp_size_t rn, const mp_limb_t* a, mp_size_t an,
                           const mp_limb_t* b, mp_size_t bn) {
    an = norm_len(a, an);
    bn = norm_len(b, bn);
    rn = norm_len(r, rn);
    for (int i = 0; i < MOD_P_CNT; ++i) {
        u32 p = MOD_P[i];
        u64 want = mulmod32(mod32(a, an, p), mod32(b, bn, p), p);
        if (mod32(r, rn, p) != want) return false;
    }
    return true;
}

// 模和检查：[r,rn] ≡ [a,an]+[b,bn] (mod p)
inline bool mod_is_sum(const mp_limb_t* r, mp_size_t rn, const mp_limb_t* a, mp_size_t an,
                       const mp_limb_t* b, mp_size_t bn) {
    an = norm_len(a, an);
    bn = norm_len(b, bn);
    rn = norm_len(r, rn);
    for (int i = 0; i < MOD_P_CNT; ++i) {
        u32 p = MOD_P[i];
        u64 want = addmod32(mod32(a, an, p), mod32(b, bn, p), p);
        if (mod32(r, rn, p) != want) return false;
    }
    return true;
}

// 模差检查：[r,rn] ≡ [a,an]-[b,bn] (mod p)（模意义下无符号差）
inline bool mod_is_diff(const mp_limb_t* r, mp_size_t rn, const mp_limb_t* a, mp_size_t an,
                        const mp_limb_t* b, mp_size_t bn) {
    an = norm_len(a, an);
    bn = norm_len(b, bn);
    rn = norm_len(r, rn);
    for (int i = 0; i < MOD_P_CNT; ++i) {
        u32 p = MOD_P[i];
        u64 want = submod32(mod32(a, an, p), mod32(b, bn, p), p);
        if (mod32(r, rn, p) != want) return false;
    }
    return true;
}

/*
    库组合判据与构造（多文件共享；全部为库原语组合，非算法实现）。
    铁律（2026-11-03）：测试禁止引入任何自建算法实现作为交叉验证 oracle
    ——无 BigInt/u128、无手写逐位乘除/开方循环、无库除法驱动的欧几里得
    循环求 gcd/gcdext 参考。允许的手段仅限：库内多路径互比、数学恒等式
    （本节的整除性/重建类判据）、写死常数、模检查、64 位内标量参考。
*/

// 整除性判据：[p,pn] mod [d,dn] == 0（恒等式必要条件，库 div_ 求余）
inline bool divides_all(const mp_limb_t* d, mp_size_t dn, const mp_limb_t* p, mp_size_t pn) {
    if (dn == 0) return false;
    mp_ptr r = (mp_ptr)lmmp_alloc((size_t)dn * sizeof(mp_limb_t));
    lmmp_div_(NULL, r, p, pn, d, dn);
    bool ok = norm_len(r, dn) == 0;
    lmmp_free(r);
    return ok;
}

// 零垫对齐加法：sum = a + b（长度取 mx；库 add_n_）
inline void pad_add(const mp_limb_t* a, mp_size_t la, const mp_limb_t* b, mp_size_t lb, mp_size_t mx,
                    mp_ptr sum) {
    lmmp_zero(sum, mx);
    memcpy(sum, a, (size_t)la * 8);
    mp_ptr pad = alloc_limbs(mx);
    lmmp_zero(pad, mx);
    memcpy(pad, b, (size_t)lb * 8);
    lmmp_add_n_(sum, sum, pad, mx);
    lmmp_free(pad);
}

// mul_ 契约 na>=nb：按长度排序后调用
inline void mul_sorted(mp_ptr dst, const mp_limb_t* a, mp_size_t la, const mp_limb_t* b,
                       mp_size_t lb) {
    if (la >= lb) lmmp_mul_(dst, a, la, b, lb);
    else lmmp_mul_(dst, b, lb, a, la);
}

// [e,en] 乘小常数 v 的链参考（库 mul_1_ 原地），返回新长度；越界返回 -1
inline mp_size_t chain_mul_small(mp_ptr e, mp_size_t en, u64 v, size_t cap) {
    u64 hi = lmmp_mul_1_(e, e, en, v);
    if (hi) {
        if ((size_t)en + 1 >= cap) return -1;
        e[en] = hi;
        return en + 1;
    }
    return en;
}

// 64 位内素数试除参考（标量简单参考允许域）
inline bool is_prime_small(u32 n) {
    if (n < 2) return false;
    for (u32 d = 2; (u64)d * d <= n; ++d)
        if (n % d == 0) return false;
    return true;
}

// 相邻斐波那契对 (F(k), F(k+1))，F(k+1) 实际长度 >= want_n；返回 F(k+1)
// 实际长度（fibonacci2_ 返回值）。缓冲上方零垫：F(k) 长度可为返回值或其减一。
inline mp_size_t fib_pair(mp_ptr x, mp_ptr y, mp_size_t buf, mp_size_t want_n) {
    ulong k = 3;
    for (;; ++k) {
        mp_size_t need = lmmp_fibonacci_size_(k + 1);
        // size_ 比实际长度多 2-4 limb：need 超 buf+4 后实际长度也不可能 <= buf
        if (need > buf + 4) return 0;
        if (need <= buf) {
            lmmp_zero(x, buf);
            lmmp_zero(y, buf);
            mp_size_t ly = lmmp_fibonacci2_(y, x, need, k);  // x=F(k), y=F(k+1)
            if (ly >= want_n) return ly;
        }
    }
}

}  // namespace lmmp_test_utils

#endif  // LMMP_TEST_UTILS_HPP

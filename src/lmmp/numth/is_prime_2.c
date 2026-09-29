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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

/*
    Granlund-Montgomery 试除表：奇素数 3..137 共 32 个，每行
      [0..1] = p^(-1) mod 2^128
      [2..3] = floor((2^128-1)/p)
    0 <= n < 2^128 可被 p 整除 <=> n * p^(-1) <= floor((2^128-1)/p)
*/
static const ulong ll_trial_primes[32][4] = {
    { 0xaaaaaaaaaaaaaaab, 0xaaaaaaaaaaaaaaaa, 0x5555555555555555, 0x5555555555555555 },  // 3
    { 0xcccccccccccccccd, 0xcccccccccccccccc, 0x3333333333333333, 0x3333333333333333 },  // 5
    { 0x6db6db6db6db6db7, 0xb6db6db6db6db6db, 0x4924924924924924, 0x2492492492492492 },  // 7
    { 0x2e8ba2e8ba2e8ba3, 0xa2e8ba2e8ba2e8ba, 0x745d1745d1745d17, 0x1745d1745d1745d1 },  // 11
    { 0x4ec4ec4ec4ec4ec5, 0xc4ec4ec4ec4ec4ec, 0x3b13b13b13b13b13, 0x13b13b13b13b13b1 },  // 13
    { 0xf0f0f0f0f0f0f0f1, 0xf0f0f0f0f0f0f0f0, 0x0f0f0f0f0f0f0f0f, 0x0f0f0f0f0f0f0f0f },  // 17
    { 0x86bca1af286bca1b, 0xbca1af286bca1af2, 0xe50d79435e50d794, 0x0d79435e50d79435 },  // 19
    { 0xd37a6f4de9bd37a7, 0x4de9bd37a6f4de9b, 0x42c8590b21642c85, 0x0b21642c8590b216 },  // 23
    { 0x34f72c234f72c235, 0xc234f72c234f72c2, 0xd3dcb08d3dcb08d3, 0x08d3dcb08d3dcb08 },  // 29
    { 0xef7bdef7bdef7bdf, 0xdef7bdef7bdef7bd, 0x8421084210842108, 0x0842108421084210 },  // 31
    { 0x14c1bacf914c1bad, 0xc1bacf914c1bacf9, 0x5306eb3e45306eb3, 0x06eb3e45306eb3e4 },  // 37
    { 0x8f9c18f9c18f9c19, 0x18f9c18f9c18f9c1, 0x63e7063e7063e706, 0x063e7063e7063e70 },  // 41
    { 0x82fa0be82fa0be83, 0xbe82fa0be82fa0be, 0xf417d05f417d05f4, 0x05f417d05f417d05 },  // 43
    { 0x51b3bea3677d46cf, 0x3677d46cefa8d9df, 0x882b9310572620ae, 0x0572620ae4c415c9 },  // 47
    { 0x21cfb2b78c13521d, 0x13521cfb2b78c135, 0x4873ecade304d487, 0x04d4873ecade304d },  // 53
    { 0xcbeea4e1a08ad8f3, 0x8f2fba9386822b63, 0x15b1e5f75270d045, 0x0456c797dd49c341 },  // 59
    { 0x4fbcda3ac10c9715, 0x14fbcda3ac10c971, 0x4325c53ef368eb04, 0x04325c53ef368eb0 },  // 61
    { 0xf0b7672a07a44c6b, 0xc2dd9ca81e9131ab, 0x40f4898d5f85bb39, 0x03d226357e16ece5 },  // 67
    { 0x193d4bb7e327a977, 0x4f52edf8c9ea5dbf, 0x240e6c2b4481cd85, 0x039b0ad12073615a },  // 71
    { 0x7e3f1f8fc7e3f1f9, 0x3f1f8fc7e3f1f8fc, 0x070381c0e070381c, 0x0381c0e070381c0e },  // 73
    { 0x9b8b577e613716af, 0xd5df984dc5abbf30, 0xa5440cf6474a8819, 0x033d91d2a2067b23 },  // 79
    { 0xa3784a062b2e43db, 0x2818acb90f6bf3a9, 0x6f0940c565c87b5f, 0x03159721ed7e7534 },  // 83
    { 0xf47e8fd1fa3f47e9, 0xd1fa3f47e8fd1fa3, 0xc0b81702e05c0b81, 0x02e05c0b81702e05 },  // 89
    { 0xa3a0fd5c5f02a3a1, 0x5f02a3a0fd5c5f02, 0xa0fd5c5f02a3a0fd, 0x02a3a0fd5c5f02a3 },  // 97
    { 0x3a4c0a237c32b16d, 0xc32b16cfd7720f35, 0xc83cd4e930288df0, 0x0288df0cac5b3f5d },  // 101
    { 0xdab7ec1dd3431b57, 0xd0c6d5bf60ee9a18, 0x88b2f392a409f116, 0x027c45979c95204f },  // 103
    { 0x77a04c8f8d28ac43, 0xa2b10bf66e0e5aea, 0xdc1cb5d4ef40991f, 0x02647c69456217ec },  // 107
    { 0xa6c0964fda6c0965, 0xc0964fda6c0964fd, 0x9b02593f69b02593, 0x02593f69b02593f6 },  // 109
    { 0x90fdbc090fdbc091, 0xc090fdbc090fdbc0, 0x43f6f0243f6f0243, 0x0243f6f0243f6f02 },  // 113
    { 0x7efdfbf7efdfbf7f, 0xbf7efdfbf7efdfbf, 0x0408102040810204, 0x0204081020408102 },  // 127
    { 0x03e88cb3c9484e2b, 0xf82ee6986d6f63aa, 0x7f05dcd30dadec75, 0x01f44659e4a42715 },  // 131
    { 0xe21a291c077975b9, 0x21a291c077975b8f, 0x701de5d6e3f8868a, 0x01de5d6e3f8868a4 },  // 137
};

/* 设 d 为最大试除素数。若 x < 2^128 - d*B，则积的高 limb 单独比较（<）与
   双 limb 比较（<=）结果一致，可省一次低位比较；近上界的输入（如从
   2^128 递减生成素数）退回全比较 */
#define FAST_TRIAL_BOUND (MP_ULONG_MAX - 137)

static inline int ll_le(u128 a, u128 b) {
    return a <= b;
}

/*
    ============ 双 limb Montgomery 核心（R = B^2，u128 标量风格） ============

    模数 n（奇，B <= n < B^2），ninv = -n^(-1) mod B^2，操作数为 < n 的
    蒙域剩余。REDC：T < n*B^2 时 r = (T + q*n)/B^2 < 2n，其中
    q = T_lo * ninv mod B^2（u128 截断乘恰为其低 128 位），至多一次
    条件减法得规范剩余。

    全程以 u128 标量表达（假定 __uint128_t 可用）：和的进位由回绕
    比较自然产生，由编译器融合为 add/adc 链；x64 下乘法借用库内调优的
    mulx/adcx 内联汇编助手，其余平台为纯 C（编译器生成 mul/umulh 链）。
*/

static inline void mul_128(u128 a, u128 b, u128 *hi, u128 *lo) {
#if defined(LMMP_ASM_X64) && (defined(__GNUC__) || defined(__clang__))
    ulong u[4];
    _umul128to256_(_u128high(a), _u128low(a), _u128high(b), _u128low(b), u);
    *lo = ((u128)u[1] << 64) | u[0];
    *hi = ((u128)u[3] << 64) | u[2];
#else
    ulong a0 = _u128low(a), a1 = _u128high(a);
    ulong b0 = _u128low(b), b1 = _u128high(b);
    u128 p00 = (u128)a0 * b0;
    u128 p01 = (u128)a0 * b1;
    u128 p10 = (u128)a1 * b0;
    u128 p11 = (u128)a1 * b1;
    u128 mid = p01 + p10;
    u128 cm = mid < p01; /* 129 位交叉和进位，属 hi 的第 64 位 */
    u128 l = (mid << 64) + p00;
    u128 cl = l < p00; /* 进位属 hi 的第 0 位 */
    *lo = l;
    *hi = p11 + (mid >> 64) + cl + (cm << 64);
#endif
}

/* a^2 -> (hi, lo)：交叉积单次乘法后整体加倍（3 乘法平方） */
static inline void sqr_128(u128 a, u128 *hi, u128 *lo) {
#if defined(LMMP_ASM_X64) && (defined(__GNUC__) || defined(__clang__))
    ulong u[4];
    _usqr128to256_(_u128high(a), _u128low(a), u);
    *lo = ((u128)u[1] << 64) | u[0];
    *hi = ((u128)u[3] << 64) | u[2];
#else
    ulong a0 = _u128low(a), a1 = _u128high(a);
    u128 p0 = (u128)a0 * a0;
    u128 m = (u128)a0 * a1; /* 交叉积，加倍后 129 位 */
    u128 p1 = (u128)a1 * a1;
    u128 ml = m << 1;
    u128 mh = m >> 127; /* 加倍进位（0/1），属 hi 的第 64 位 */
    u128 l = (ml << 64) + p0;
    u128 cl = l < p0;
    *lo = l;
    *hi = p1 + (ml >> 64) + cl + (mh << 64);
#endif
}

/* REDC：r = (T + q*n)/B^2，T = thi*B^2 + tlo < n*B^2。n 接近 B^2 时和
   可达 129 位，回绕进位下按模 2^128 回绕减 n 恰为真值且已 < n */
static inline u128 mont2_redc(u128 thi, u128 tlo, u128 n, u128 ninv) {
    u128 q = tlo * ninv; /* u128 截断乘即 mod B^2 */
    u128 qh, ql, lo, hi, c, c1, c2;
    mul_128(q, n, &qh, &ql);
    lo = tlo + ql;
    c = lo < tlo;
    hi = thi + qh;
    c1 = hi < thi;
    hi += c;
    c2 = c && hi == 0;
    if (c1 | c2)
        hi -= n;
    else if (hi >= n)
        hi -= n;
    return hi;
}

static inline u128 mont2_sqr(u128 a, u128 n, u128 ninv) {
    u128 h, l;
    sqr_128(a, &h, &l);
    return mont2_redc(h, l, n, ninv);
}

static inline u128 mont2_mul_2(u128 a, u128 b, u128 n, u128 ninv) {
    u128 h, l;
    mul_128(a, b, &h, &l);
    return mont2_redc(h, l, n, ninv);
}

/* REDC(a*b)，b 为单 limb：a 为蒙域剩余时结果出域（a_plain*b），
   专用于转域 */
static inline u128 mont2_redc_mul_1(u128 a, ulong b, u128 n, u128 ninv) {
    /* T = a*b < 2^192：h<<64 回绕丢弃的高位入 thi，低位和的进位并入 thi */
    u128 lo = (u128)_u128low(a) * b;
    u128 h = (u128)_u128high(a) * b;
    u128 tlo = lo + (h << 64);
    return mont2_redc((h >> 64) + (tlo < lo), tlo, n, ninv);
}

/* 转蒙域：x*R mod n = REDC(x*R2)，R2 = B^4 mod n（x 为小整数） */
static inline u128 mont2_to_mont1(ulong x, u128 R2, u128 n, u128 ninv) {
    return mont2_redc_mul_1(R2, x, n, ninv);
}

static inline u128 mont2_to_mont2(u128 a, u128 R2, u128 n, u128 ninv) {
    return mont2_mul_2(a, R2, n, ninv);
}

/* 蒙域加倍/模减/模取负（操作数 < n，结果 < n）：
   129 位和的回绕进位下，回绕减 n 恰为真值且已 < n */
static inline u128 mont2_dbl(u128 a, u128 n) {
    u128 s = a + a;
    if (s < a)
        s -= n;
    else if (s >= n)
        s -= n;
    return s;
}

static inline u128 mont2_sub(u128 a, u128 b, u128 n) {
    u128 r = a - b;
    if (a < b) r += n;
    return r;
}

static inline u128 mont2_neg(u128 a, u128 n) {
    return a == 0 ? a : n - a;
}

/* u128 有效位数（最高位以下位数），x != 0 */
static inline int bitlen_2(u128 x) {
    if (_u128high(x) != 0) return 128 - lmmp_leading_zeros_(_u128high(x));
    return 64 - lmmp_leading_zeros_(_u128low(x));
}

/* n - 1 = d * 2^t（d 奇）；n + 1 同型 */
static inline u128 factor_2adic(u128 x, slong *t) {
    if (_u128low(x) == 0) {
        *t = 64 + lmmp_tailing_zeros_(_u128high(x));
        return (u128)_u128high(x) >> (*t - 64);
    }
    *t = lmmp_tailing_zeros_(_u128low(x));
    return x >> *t;
}

/* (b|A) 二进制算法：A 奇，b < A < 2^16（移位/比较/减法，几次迭代） */
static int jacobi_small(ulong b, ulong A) {
    int sign = 1;
    if (b == 0) return 0;
    for (;;) {
        int tz = lmmp_tailing_zeros_(b);
        b >>= tz;
        if ((tz & 1) && ((A & 7) == 3 || (A & 7) == 5)) sign = -sign;
        if (b == 1) return sign;
        if (b < A) {
            ulong s = b;
            b = A;
            A = s;
            if ((b & 3) == 3 && (A & 3) == 3) sign = -sign;
        }
        b -= A;
        if (b == 0) return 0;
    }
}

/*
    (±Dabs|n)：Dabs 奇 < 2^16，n 128 位奇。n 以双 limb 模乘链折叠
    （r = (r*(B mod A) + limb) mod A，全程 64 位），再经二次互反律
    (A|n) = (n|A)*(-1)^(((A-1)/2)((n-1)/2)) 归结到 jacobi_small；
    D 为负时乘 (-1|n) 因子。gcd(|D|,n)>1 返回 0
*/
static int jacobi_D(uint Dabs, int Dneg, u128 n) {
    ulong A = Dabs;
    ulong Bm = (ulong)-1 % A + 1;
    ulong b;
    int sign = 1;
    if (Bm >= A) Bm -= A; /* Bm = 2^64 mod A */
    b = _u128high(n) % A;
    b = (b * Bm + _u128low(n) % A) % A;
    if ((A & 3) == 3 && (_u128low(n) & 3) == 3) sign = -sign;
    sign *= jacobi_small(b, A);
    if (sign == 0) return 0;
    if (Dneg && (_u128low(n) & 3) == 3) sign = -sign;
    return sign;
}

/* 二次探测第二阶段：y = b^d 已得，检查 y ∈ {1, n-1} 或 y^(2^j) = n-1
   (0 < j < t)。one/m_1 均为蒙域剩余 */
static inline int mont2_sprp_stage2(u128 y, slong t, u128 one, u128 m_1, u128 n, u128 ninv) {
    if (y == one || y == m_1) return 1;
    for (t--; t > 0; t--) {
        y = mont2_sqr(y, n, ninv);
        if (y == m_1) return 1;
    }
    return 0;
}

/*
    基底 2 专用幂 y = 2^d（蒙域）：指数最高 k 位（k <= 6）对应的 2^t
   （t < 2^6，普通域一次移位）经一次 REDC 跳启动，其后逐位平方，
   逢 1 蒙域加倍。ebits = d 的有效位数
*/
static u128 mont2_powmod_2_(u128 d, int ebits, u128 R2, u128 n, u128 ninv) {
    int k = ebits > 6 ? 6 : ebits;
    int bit;
    u128 v = mont2_to_mont1((ulong)1 << (ulong)(d >> (ebits - k)), R2, n, ninv);
    for (bit = ebits - k - 1; bit >= 0; bit--) {
        v = mont2_sqr(v, n, ninv);
        if ((d >> bit) & 1) v = mont2_dbl(v, n);
    }
    return v;
}

/* 三基底交错幂 y1 = b1^d, y2 = b2^d, y3 = b3^d（蒙域）：
   3 条独立平方链掩盖模乘延迟；乘基底须用蒙域值 bm*，普通域小数
   直接参与 REDC 会把链拉出蒙域 */
static void mont2_powmod_3_(u128 *y1, u128 *y2, u128 *y3, ulong b1, ulong b2, ulong b3, u128 d,
                            int ebits, u128 R2, u128 n, u128 ninv) {
    u128 x, v, w, bm1, bm2, bm3;
    int bit;

    x = mont2_to_mont1(b1, R2, n, ninv);
    v = mont2_to_mont1(b2, R2, n, ninv);
    w = mont2_to_mont1(b3, R2, n, ninv);
    bm1 = x;
    bm2 = v;
    bm3 = w;

    for (bit = ebits - 2; bit >= 0; bit--) {
        x = mont2_sqr(x, n, ninv);
        v = mont2_sqr(v, n, ninv);
        w = mont2_sqr(w, n, ninv);
        if ((d >> bit) & 1) {
            x = mont2_mul_2(x, bm1, n, ninv);
            v = mont2_mul_2(v, bm2, n, ninv);
            w = mont2_mul_2(w, bm3, n, ninv);
        }
    }
    *y1 = x;
    *y2 = v;
    *y3 = w;
}

/*
    12 个奇基底 Rabin-Miller（基底 3..41，3 路交错），基底 2 已由调用
    方先行完成（d/ebits/t/one/m_1 复用）。n < psi_13 时与前 13 个素数
    基底共同构成 Sorenson-Webster 确定性判据
*/
static int mont2_is_prime_mr13(u128 n, u128 ninv, u128 R2, u128 d, int ebits, slong t, u128 one,
                               u128 m_1) {
    u128 y1, y2, y3;
    static const uchar bases[12] = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41};
    int j;

    for (j = 0; j < 12; j += 3) {
        mont2_powmod_3_(&y1, &y2, &y3, bases[j], bases[j + 1], bases[j + 2], d, ebits, R2, n,
                        ninv);
        if (!mont2_sprp_stage2(y1, t, one, m_1, n, ninv)) return 0;
        if (!mont2_sprp_stage2(y2, t, one, m_1, n, ninv)) return 0;
        if (!mont2_sprp_stage2(y3, t, one, m_1, n, ninv)) return 0;
    }
    return 1;
}

/*
    ============ 强 Lucas-Selfridge 测试（V-only 阶梯） ============

    Selfridge 方法 A：P = 1，Q = (1-D)/4，D 取 5, -7, 9, -11, ... 中
    首个 (D|n) = -1 者。V 阶梯递推（蒙域）：
      V_{2k}   = V_k^2 - 2Q^k
      V_{2k+1} = V_k*V_{k+1} - Q^k
      V_{2k+2} = V_{k+1}^2 - 2Q^{k+1}
    Q 链同步阶梯：(Q^k, Q^{k+1}) --bit0--> ((Q^k)^2, (Q^k)^2*Q)，
    --bit1--> (Q^k*Q^{k+1}, (Q^{k+1})^2)。
    判据（gcd(D,n)=1 下与 U/V 强 Lucas 判据等价，U_d = 0 经恒等式
    V_d^2 - D*U_d^2 = 4Q^d 转为 V_d^2 = 4Q^d）：
      V_d = 0，或 V_d^2 = 4Q^d，或 V_{d*2^r} = 0（0 < r < s），
    d*2^s = n+1。通过返回 1；判定合数（含 gcd(|D|,n)>1）返回 0
*/
static int mont2_lucas_strong_(u128 n, u128 ninv, u128 R2) {
    u128 one, Qm, Qk, Qk1, Vk, Vk1, d, u, w, r;
    slong s, i;
    int j, Dneg;
    uint Dabs, Qabs;

    /* 寻找 D：素数 n 的最小符合条件的 |D| 极小（< 2*log2(n)^2 量级），
       上界 201 已远超已知记录；超界仍无 D 视为合数（如完全平方数） */
    Dabs = 5;
    Dneg = 0;
    for (;;) {
        j = jacobi_D(Dabs, Dneg, n);
        if (j == 0) return 0;
        if (j == -1) break;
        Dabs += 2;
        Dneg ^= 1;
        if (Dabs > 201) return 0;
    }

    /* Q = (1-D)/4，可为负；d*2^s = n+1（n 奇，n+1 偶） */
    Qabs = Dneg ? (Dabs + 1) / 4 : (Dabs - 1) / 4;
    d = factor_2adic(n + 1, &s);

    one = mont2_to_mont1(1, R2, n, ninv);
    Qm = mont2_to_mont1(Qabs, R2, n, ninv);
    if (!Dneg) Qm = mont2_neg(Qm, n);

    /* 阶梯初态 k=1：(V_1, V_2, Q^1, Q^2) = (1, 1-2Q, Q, Q^2) */
    Vk = one;
    Vk1 = mont2_sub(one, mont2_dbl(Qm, n), n);
    Qk = Qm;
    Qk1 = mont2_sqr(Qm, n, ninv);

    for (i = bitlen_2(d) - 2; i >= 0; i--) {
        if ((d >> i) & 1) {
            u = mont2_mul_2(Vk, Vk1, n, ninv); /* V_{2k+1} 主项 */
            w = mont2_sqr(Vk1, n, ninv);       /* V_{2k+2} 主项 */
            r = mont2_dbl(Qk1, n);
            Vk = mont2_sub(u, Qk, n);
            Vk1 = mont2_sub(w, r, n);
            Qk = mont2_mul_2(Qk, Qk1, n, ninv); /* Q^{2k+1} */
            Qk1 = mont2_sqr(Qk1, n, ninv);      /* Q^{2k+2} */
        } else {
            u = mont2_sqr(Vk, n, ninv);       /* V_{2k} 主项 */
            w = mont2_mul_2(Vk, Vk1, n, ninv); /* V_{2k+1} 主项 */
            r = mont2_dbl(Qk, n);
            Vk = mont2_sub(u, r, n);
            Vk1 = mont2_sub(w, Qk, n);
            w = mont2_sqr(Qk, n, ninv);       /* Q^{2k} */
            Qk1 = mont2_mul_2(w, Qm, n, ninv); /* Q^{2k+1} */
            Qk = w;
        }
    }

    /* 判据 */
    if (Vk == 0) return 1;
    u = mont2_sqr(Vk, n, ninv);
    r = mont2_dbl(mont2_dbl(Qk, n), n); /* 4Q^d */
    if (u == r) return 1;
    for (s--; s > 0; s--) {
        r = mont2_dbl(Qk, n);
        Vk = mont2_sub(u, r, n); /* V_{2d} = V_d^2 - 2Q^d（复用 u = V_d^2） */
        Qk = mont2_sqr(Qk, n, ninv);
        u = mont2_sqr(Vk, n, ninv);
        if (Vk == 0) return 1;
    }
    return 0;
}

/* 蒙域参数预计算：ninv = -n^(-1) mod B^2，R2 = B^4 mod n
   （[nn,5] = 2^256 除以 n 的余数） */
static void mont2_preinv(u128 *ninv, u128 *R2, u128 n) {
    ulong i2[2], q[4], nn[5] = {0, 0, 0, 0, 1};
    ulong n2[2];

    i2[0] = _u128low(n);
    i2[1] = _u128high(n);
    n2[0] = i2[0];
    n2[1] = i2[1];
    lmmp_binvert_2_(i2, i2);
    i2[0] = -i2[0];
    i2[1] = ~i2[1] + (i2[0] == 0);
    lmmp_div_2_(q, nn, 5, n2);
    *ninv = ((u128)i2[1] << 64) | i2[0];
    *R2 = ((u128)n2[1] << 64) | n2[0];
}

int lmmp_is_prime_2_(mp_limb_t lo, mp_limb_t hi) {
    lmmp_param_assert(hi > 0);
    u128 n, ninv, R2, nm1, d, one, m_1, y;
    slong t, i;
    int ebits, full;

    if ((lo & 1) == 0) return 0;

#define ainv0 ll_trial_primes[i][0]
#define ainv1 ll_trial_primes[i][1]
#define abound0 ll_trial_primes[i][2]
#define abound1 ll_trial_primes[i][3]
#define binv0 ll_trial_primes[i + 1][0]
#define binv1 ll_trial_primes[i + 1][1]
#define bbound0 ll_trial_primes[i + 1][2]
#define bbound1 ll_trial_primes[i + 1][3]

    if (hi < FAST_TRIAL_BOUND) {
        for (i = 0; i < 32; i += 2) {
            u128 a = _umul128to128_(hi, lo, ainv1, ainv0);
            u128 b = _umul128to128_(hi, lo, binv1, binv0);
            if ((_u128high(a) < abound1) | (_u128high(b) < bbound1)) return 0;
        }
    } else {
        for (i = 0; i < 32; i += 2) {
            u128 a = _umul128to128_(hi, lo, ainv1, ainv0);
            u128 b = _umul128to128_(hi, lo, binv1, binv0);
            if (ll_le(a, ((u128)abound1 << 64) | abound0) | ll_le(b, ((u128)bbound1 << 64) | bbound0))
                return 0;
        }
    }
#undef ainv0
#undef ainv1
#undef abound0
#undef abound1
#undef binv0
#undef binv1
#undef bbound0
#undef bbound1

    n = ((u128)hi << 64) | lo;
    mont2_preinv(&ninv, &R2, n);

    /* SWbound = psi_13 (Sorenson-Webster)*/
    full = n < ((((u128)0x2be69) << 64) | 0x51adc5b22410a5fd);

    nm1 = n - 1;
    d = factor_2adic(nm1, &t);
    ebits = bitlen_2(d);
    one = mont2_to_mont1(1, R2, n, ninv);
    m_1 = mont2_to_mont2(nm1, R2, n, ninv);

    y = mont2_powmod_2_(d, ebits, R2, n, ninv);
    if (!mont2_sprp_stage2(y, t, one, m_1, n, ninv)) return 0;

    if (full) return mont2_is_prime_mr13(n, ninv, R2, d, ebits, t, one, m_1) ? 1 : 0;

    return mont2_lucas_strong_(n, ninv, R2) ? 2 : 0;
}

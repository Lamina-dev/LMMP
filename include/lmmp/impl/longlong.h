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

#ifndef __LMMP_LONGLONG_H__
#define __LMMP_LONGLONG_H__

#if defined(LMMP_ASM_X64) && (defined(__GNUC__) || defined(__clang__))
#include <x86intrin.h>
#endif

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
// cnt = ctz(x)
// r = x >> cnt
// assume x is non-zero
#define ctz_shr_u64(r, x, cnt)        \
    do {                              \
        unsigned long long _x_ = (x); \
        (cnt) = __builtin_ctzll(_x_); \
        (r) = _x_ >> (cnt);           \
    } while (0)
// cnt = clz(x)
// r = x << cnt
// assume x is non-zero
#define clz_shl_u64(r, x, cnt)                    \
    do {                                          \
        unsigned long long _x_ = (x);             \
        (cnt) = __builtin_clzll(_x_);             \
        (r) = (unsigned long long)(_x_ << (cnt)); \
    } while (0)
#else
// cnt = ctz(x)
// r = x >> cnt
// assume x is non-zero
#define ctz_shr_u64(r, x, cnt)   \
    do {                         \
        uint64_t _x_ = (x);      \
        int _i_ = 0;             \
        while ((_x_ & 1) == 0) { \
            _i_++;               \
            _x_ >>= 1;           \
        }                        \
        cnt = _i_;               \
        (r) = _x_;               \
    } while (0)
// cnt = clz(x)
// r = x << cnt
// assume x is non-zero
#define clz_shl_u64(r, x, cnt)                       \
    do {                                             \
        uint64_t _x_ = (x);                          \
        int _c_ = 0;                                 \
        while ((_x_ & 0x8000000000000000ULL) == 0) { \
            _c_++;                                   \
            _x_ <<= 1;                               \
        }                                            \
        (cnt) = _c_;                                 \
        (r) = _x_;                                   \
    } while (0)
#endif

#if defined(__GNUC__) || defined(__clang__)
// cnt = ctz(x)
// r = x >> cnt
// assume x is non-zero
#define ctz_shr_u32(r, x, cnt)      \
    do {                            \
        unsigned int _x_ = (x);     \
        (cnt) = __builtin_ctz(_x_); \
        (r) = _x_ >> (cnt);         \
    } while (0)
// cnt = clz(x)
// r = x << cnt
// assume x is non-zero
#define clz_shl_u32(r, x, cnt)      \
    do {                            \
        unsigned int _x_ = (x);     \
        (cnt) = __builtin_clz(_x_); \
        (r) = _x_ << (cnt);         \
    } while (0)
#else
// cnt = ctz(x)
// r = x >> cnt
// assume x is non-zero
#define ctz_shr_u32(r, x, cnt)    \
    do {                          \
        uint32_t _x_ = (x);       \
        int _i_ = 0;              \
        while ((_x_ & 1U) == 0) { \
            _i_++;                \
            _x_ >>= 1;            \
        }                         \
        cnt = _i_;                \
        (r) = _x_;                \
    } while (0)
// cnt = clz(x)
// r = x << cnt
// assume x is non-zero
#define clz_shl_u32(r, x, cnt)             \
    do {                                   \
        uint32_t _x_ = (x);                \
        int _c_ = 0;                       \
        while ((_x_ & 0x80000000U) == 0) { \
            _c_++;                         \
            _x_ <<= 1;                     \
        }                                  \
        (cnt) = _c_;                       \
        (r) = _x_;                         \
    } while (0)
#endif

static inline void _umul64to128_(uint64_t a, uint64_t b, uint64_t *low, uint64_t *high) {
#if (defined(__GNUC__) || defined(__clang__))
    __uint128_t prod = (__uint128_t)a * b;
    *low = (uint64_t)prod;
    *high = (uint64_t)(prod >> 64);
#else
    uint64_t ah = a >> 32, bh = b >> 32;
    a = (uint32_t)a, b = (uint32_t)b;
    uint64_t r0 = a * b, r1 = a * bh, r2 = ah * b, r3 = ah * bh;
    r3 += (r1 >> 32) + (r2 >> 32);
    r1 = (uint32_t)r1, r2 = (uint32_t)r2;
    r1 += r2;
    r1 += (r0 >> 32);
    *high = r3 + (r1 >> 32);
    *low = (r1 << 32) | (uint32_t)r0;
#endif
}

static inline uint64_t _umul64to64hi_(uint64_t a, uint64_t b) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
    __uint128_t t = (__uint128_t)a * (__uint128_t)b;
    return (uint64_t)(t >> 64);
#else
    uint64_t ah = a >> 32, bh = b >> 32;
    a = (uint32_t)a, b = (uint32_t)b;
    uint64_t r0 = a * b, r1 = a * bh, r2 = ah * b, r3 = ah * bh;
    r3 += (r1 >> 32) + (r2 >> 32);
    r1 = (uint32_t)r1, r2 = (uint32_t)r2;
    r1 += r2;
    r1 += (r0 >> 32);
    return r3 + (r1 >> 32);
#endif
}

static inline void _umul128to256_(uint64_t a_high, uint64_t a_low, uint64_t b_high, uint64_t b_low, uint64_t rr[4]) {
    /*
        | res0 | res1 | res2 | res3 |
        |  p0l |  p0h |      |      |
               |  p1l |  p1h |      |
               |  p2l |  p2h |      |
               |      |  p3l |  p3h |
    */
#if defined(LMMP_ASM_X64) && (defined(__GNUC__) || defined(__clang__))
    /* 4×mulx 独立发射；交叉积之和 t = p1+p2 为 129 位（b 为溢出位，
       由 setc 提取到已死亡的 p2l 寄存器），链尾 res3 = p3h + b + c2 收拢。 */
    uint64_t p0l, t_l, t_h, p3l, p2l, p2h, r1, r2, r3;
    __asm__(
        "movq   %[al], %%rdx            \n\t"
        "mulxq  %[bl], %[p0l], %[r1]    \n\t"  // p0 = al*bl, r1 = p0h
        "mulxq  %[bh], %[t_l], %[t_h]   \n\t"  // p1 = al*bh
        "movq   %[ah], %%rdx            \n\t"
        "mulxq  %[bl], %[p2l], %[p2h]   \n\t"  // p2 = ah*bl
        "mulxq  %[bh], %[p3l], %[r3]    \n\t"  // p3 = ah*bh, r3 = p3h
        "addq   %[p2l], %[t_l]          \n\t"  // t_l = p1l + p2l, CF = a
        "adcxq  %[p2h], %[t_h]          \n\t"  // t_h = p1h + p2h + a, CF = b
        "movl   $0, %k[p2l]             \n\t"  // 不动标志，清零待 setc
        "setcb  %b[p2l]                 \n\t"  // p2l = b
        "addq   %[t_l], %[r1]           \n\t"  // r1 = p0h + t_l = res1, CF = c1
        "movq   %[t_h], %[r2]           \n\t"
        "adcxq  %[p3l], %[r2]           \n\t"  // r2 = t_h + p3l + c1 = res2, CF = c2
        "adcxq  %[p2l], %[r3]           \n\t"  // r3 = p3h + b + c2 = res3
        : [p0l] "=&r"(p0l), [t_l] "=&r"(t_l), [t_h] "=&r"(t_h), [p3l] "=&r"(p3l),
          [p2l] "=&r"(p2l), [p2h] "=&r"(p2h), [r1] "=&r"(r1), [r2] "=&r"(r2),
          [r3] "=&r"(r3)
        : [ah] "rm"(a_high), [al] "rm"(a_low), [bh] "rm"(b_high), [bl] "rm"(b_low)
        : "cc", "rdx");
    rr[0] = p0l;
    rr[1] = r1;
    rr[2] = r2;
    rr[3] = r3;
#else
    /* 纯 C 路径：__uint128_t 组合写法在 GCC 下生成代码反而劣于显式
       limb 进位链（128 位中间值引发寄存器溢出与额外 mov），保留朴素结构。 */
    uint64_t p1_low, p1_high;  // p1 = a_low × b_high
    uint64_t p2_low, p2_high;  // p2 = a_high × b_low
    _umul64to128_(a_low, b_low, rr, rr + 1);
    _umul64to128_(a_low, b_high, &p1_low, &p1_high);
    _umul64to128_(a_high, b_low, &p2_low, &p2_high);
    _umul64to128_(a_high, b_high, rr + 2, rr + 3);
    rr[1] += p1_low;
    uint64_t carry = (rr[1] < p1_low) ? 1 : 0;
    rr[1] += p2_low;
    carry += (rr[1] < p2_low) ? 1 : 0;

    rr[2] += carry;
    carry = (rr[2] < carry) ? 1 : 0;
    rr[2] += p1_high;
    carry += (rr[2] < p1_high) ? 1 : 0;
    rr[2] += p2_high;
    carry += (rr[2] < p2_high) ? 1 : 0;

    rr[3] += carry;
#endif
}

static inline void _usqr128to256_(uint64_t a_high, uint64_t a_low, uint64_t rr[4]) {
    /*
        | res0 | res1 | res2 | res3 |
        |  p0l |  p0h |      |      |
               |  p1l |  p1h |      |
               |  p1l |  p1h |      |
               |      |  p3l |  p3h |
    */
#if defined(LMMP_ASM_X64) && (defined(__GNUC__) || defined(__clang__))
    /* 3×mulx；交叉积 p1 左移一位成为 129 位（v 为溢出位），
       shld 一条完成高位拼接，adcx/adox 双链吸收 v 与两段进位。 */
    uint64_t p0l, t_l, t_h, p3l, r1, r2, r3, z;
    __asm__(
        "movq   %[al], %%rdx            \n\t"
        "mulxq  %[al], %[p0l], %[r1]    \n\t"  // p0 = al*al, r1 = p0h
        "mulxq  %[ah], %[t_l], %[t_h]   \n\t"  // t = al*ah
        "movq   %[ah], %%rdx            \n\t"
        "mulxq  %[ah], %[p3l], %[r3]    \n\t"  // p3 = ah*ah, r3 = p3h
        "movq   %[t_h], %[r2]           \n\t"
        "shldq  $1, %[t_l], %[r2]       \n\t"  // r2 = t_h<<1 | t_l>>63
        "shrq   $63, %[t_h]             \n\t"  // t_h = v（129 位溢出，属于 res3 段）
        "leaq   (%[t_l], %[t_l]), %[t_l]\n\t"  // t_l = 2*al*ah 低 64 位（lea 不动标志）
        "xorq   %[z], %[z]              \n\t"  // CF = OF = 0（须在移位类指令之后）
        "adcxq  %[t_l], %[r1]           \n\t"  // r1 = p0h + t_l, CF = res2 进位
        "adoxq  %[p3l], %[r2]           \n\t"  // r2 += p3l, OF = res3 进位
        "adcxq  %[z], %[r2]             \n\t"  // r2 += CF = res2
        "adoxq  %[t_h], %[r3]           \n\t"  // r3 = p3h + v + OF = res3
        : [p0l] "=&r"(p0l), [t_l] "=&r"(t_l), [t_h] "=&r"(t_h), [p3l] "=&r"(p3l),
          [r1] "=&r"(r1), [r2] "=&r"(r2), [r3] "=&r"(r3), [z] "=&r"(z)
        : [ah] "rm"(a_high), [al] "rm"(a_low)
        : "cc", "rdx");
    rr[0] = p0l;
    rr[1] = r1;
    rr[2] = r2;
    rr[3] = r3;
#else
    uint64_t p1_low, p1_high;  // p1 = a_low × a_high
    _umul64to128_(a_low, a_low, rr, rr + 1);
    _umul64to128_(a_low, a_high, &p1_low, &p1_high);
    _umul64to128_(a_high, a_high, rr + 2, rr + 3);
    rr[3] += p1_high >> 63;
    p1_high = (p1_high << 1) | (p1_low >> 63);
    p1_low <<= 1;
    rr[1] += p1_low;
    uint64_t carry = (rr[1] < p1_low) ? 1 : 0;

    rr[2] += carry;
    carry = (rr[2] < carry) ? 1 : 0;
    rr[2] += p1_high;
    carry += (rr[2] < p1_high) ? 1 : 0;

    rr[3] += carry;
#endif
}

// r = 低 128 位 (a × b)，a、b 为 128 位（高/低 limb 传入）
static inline __uint128_t _umul128to128_(uint64_t a_high, uint64_t a_low, uint64_t b_high, uint64_t b_low) {
    // al*bh + ah*bl 的 64 位回绕和恰为交叉贡献的可见部分
    return (__uint128_t)a_low * b_low + ((__uint128_t)(uint64_t)(a_low * b_high + a_high * b_low) << 64);
}

static inline uint64_t _udiv128by64to64_(uint64_t numhi, uint64_t numlo, uint64_t den, uint64_t* r) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(LMMP_ASM_X64)
    uint64_t result;
    __asm__("div %[v]" : "=a"(result), "=d"(*r) : [v] "r"(den), "a"(numlo), "d"(numhi));
    return result;
#else
    const uint64_t b = ((uint64_t)1 << 32);

    uint32_t q1;
    uint32_t q0;

    uint64_t q;

    int shift;

    uint64_t den10 = den;
    uint64_t num10 = numlo;

    uint32_t den1;
    uint32_t den0;
    uint32_t num1;
    uint32_t num0;

    uint64_t rem;

    uint64_t qhat;
    uint64_t rhat;

    uint64_t c1;
    uint64_t c2;

    if (numhi >= den) {
        if (r)
            *r = ~0ull;
        return ~0ull;
    }

    clz_shl_u64(den, den, shift);
    numhi <<= shift;
    numhi |= (numlo >> (-shift & 63)) & (uint64_t)(-(int64_t)shift >> 63);
    numlo <<= shift;

    num1 = (uint32_t)(numlo >> 32);
    num0 = (uint32_t)(numlo & 0xFFFFFFFFu);
    den1 = (uint32_t)(den >> 32);
    den0 = (uint32_t)(den & 0xFFFFFFFFu);

    qhat = numhi / den1;
    rhat = numhi % den1;
    c1 = qhat * den0;
    c2 = rhat * b + num1;
    if (c1 > c2)
        qhat -= (c1 - c2 > den) ? 2 : 1;
    q1 = (uint32_t)qhat;

    rem = numhi * b + num1 - q1 * den;

    qhat = rem / den1;
    rhat = rem % den1;
    c1 = qhat * den0;
    c2 = rhat * b + num0;
    if (c1 > c2)
        qhat -= (c1 - c2 > den) ? 2 : 1;
    q0 = (uint32_t)qhat;

    q = ((uint64_t)q1 << 32) | q0;

    if (r)
        *r = num10 - q * den10;
    return q;
#endif
}

/*
    128 位辅助类型与运算（标量形式）。

    u128 直接使用编译器内建 128 位无符号标量（GCC/Clang 均提供），
    加减/比较/移位由编译器生成 add/adc、sub/sbb、shrd 等指令对，
    寄存器分配与 64 位标量同等对待。limb 数组与标量的转换见
    _u128load/_u128store（用于函数参数指针等内存场景）。

    注意：禁止对 u128 使用 / 与 % 运算符——部分平台的编译器会为此
    引入 __udivti3/__umodti3 运行时库依赖（如 Windows 下的 clang 需
    额外链接 compiler-rt）。128 位除法一律使用 _udiv128by64to64_、
    _udiv_qrnnd_preinv、_udiv_qr_3by2 等既有接口。
*/
#if defined(__SIZEOF_INT128__)

typedef __uint128_t u128;

// limb 提取（x 为 u128 标量）
#define _u128low(x) ((uint64_t)(x))

#define _u128high(x) ((uint64_t)((x) >> 64))

// 内存（limb 数组，小端序）与标量之间的装载/写回，用于函数参数指针
static inline __uint128_t _u128load(const uint64_t* p) {
    return ((__uint128_t)p[1] << 64) | p[0];
}

static inline void _u128store(uint64_t* p, __uint128_t x) {
    p[0] = (uint64_t)x;
    p[1] = (uint64_t)(x >> 64);
}

#else
#error "u128 scalar helpers require __uint128_t (GCC/Clang 64-bit targets)"
#endif

#define _add_ssaaaa(sh, sl, ah, al, bh, bl) \
    do {                                    \
        uint64_t _x_;                       \
        _x_ = (al) + (bl);                  \
        (sh) = (ah) + (bh) + (_x_ < (al));  \
        (sl) = _x_;                         \
    } while (0)

#define _sub_ddmmss(sh, sl, ah, al, bh, bl) \
    do {                                    \
        uint64_t _x_;                       \
        _x_ = (al) - (bl);                  \
        (sh) = (ah) - (bh) - ((al) < (bl)); \
        (sl) = _x_;                         \
    } while (0)

// n = nh * B + nl, di = lmmp_inv_1_(d)
// q = n / d, r = n % d
#define _udiv_qrnnd_preinv(q, r, nh, nl, d, di)                              \
    do {                                                                     \
        __uint128_t _s_ = (__uint128_t)(mp_limb_t)(nh) * (mp_limb_t)(di);    \
        _s_ += ((__uint128_t)(mp_limb_t)((nh) + 1) << 64) | (mp_limb_t)(nl); \
        mp_limb_t _qh_ = (mp_limb_t)(_s_ >> 64);                             \
        mp_limb_t _ql_ = (mp_limb_t)_s_;                                     \
        mp_limb_t _r_ = (mp_limb_t)(nl) - _qh_ * (mp_limb_t)(d);             \
        mp_limb_t _mask_ = -(mp_limb_t)(_r_ > _ql_);                         \
        _qh_ += _mask_;                                                      \
        _r_ += _mask_ & (mp_limb_t)(d);                                      \
        if (_r_ >= (mp_limb_t)(d)) {                                         \
            _r_ -= (mp_limb_t)(d);                                           \
            _qh_++;                                                          \
        }                                                                    \
        (r) = _r_;                                                           \
        (q) = _qh_;                                                          \
    } while (0)

// n = n2 * B^2 + n1 * B + n0, d = d1 * B + d0, dinv = lmmp_inv_2_1_(d)
// q = n / d, r = n % d
#define _udiv_qr_3by2(q, r1, r0, n2, n1, n0, d1, d0, dinv)                  \
    do {                                                                    \
        mp_limb_t _t0_, _t1_, _mask_;                                       \
        __uint128_t _s_ = (__uint128_t)(mp_limb_t)(n2) * (mp_limb_t)(dinv); \
        _s_ += ((__uint128_t)(mp_limb_t)(n2) << 64) | (mp_limb_t)(n1);      \
        mp_limb_t _q0_ = (mp_limb_t)_s_;                                    \
        (q) = (mp_limb_t)(_s_ >> 64);                                       \
        /* Compute the two most significant limbs of n - q'd */             \
        (r1) = (n1) - (d1) * (q);                                           \
        _sub_ddmmss((r1), (r0), (r1), (n0), (d1), (d0));                    \
        _umul64to128_((d0), (q), &_t0_, &_t1_);                             \
        _sub_ddmmss((r1), (r0), (r1), (r0), _t1_, _t0_);                    \
        (q)++;                                                              \
        /* Conditionally adjust q and the remainders */                     \
        _mask_ = -(uint64_t)((r1) >= _q0_);                                 \
        (q) += _mask_;                                                      \
        _add_ssaaaa((r1), (r0), (r1), (r0), _mask_ & (d1), _mask_ & (d0));  \
        if ((r1) >= (d1)) {                                                 \
            if ((r1) > (d1) || (r0) >= (d0)) {                              \
                (q)++;                                                      \
                _sub_ddmmss((r1), (r0), (r1), (r0), (d1), (d0));            \
            }                                                               \
        }                                                                   \
    } while (0)

// q = n0 / d0, assuming d0 is a 32-bit number, d0 > 1
// dinv = (B-1)//d0 + 1
#define _udiv32by32_q_preinv(q, n0, dinv)          \
    do {                                           \
        uint64_t _hi_, _lo_;                       \
        _umul64to128_((n0), (dinv), &_lo_, &_hi_); \
        (q) = _hi_;                                \
    } while (0)

/******************************
from https://libdivide.com/
******************************/

#define _U64_SHIFT_MASK 0x3F
#define _ADD_MARKER 0x40

typedef struct _udiv64_t {
    uint64_t magic;
    uint8_t more;
} _udiv64_t;

// assert d != 0
static inline _udiv64_t _udiv64_gen_internal_(uint64_t d, int branchfree) {
    _udiv64_t result;
    int shift;
    uint64_t t;
    clz_shl_u64(t, d, shift);
    (void)t;
    uint32_t floor_log_2_d = 63 - shift;

    // Power of 2
    if ((d & (d - 1)) == 0) {
        // We need to subtract 1 from the shift value in case of an unsigned
        // branchfree divider because there is a hardcoded right shift by 1
        // in its division algorithm. Because of this we also need to add back
        // 1 in its recovery algorithm.
        result.magic = 0;
        result.more = (uint8_t)(floor_log_2_d - (branchfree != 0));
    } else {
        uint64_t proposed_m, rem;
        uint8_t more;
        // (1 << (64 + floor_log_2_d)) / d
        proposed_m = _udiv128by64to64_((uint64_t)1 << floor_log_2_d, 0, d, &rem);

        const uint64_t e = d - rem;

        // This power works if e < 2**floor_log_2_d.
        if (!branchfree && e < ((uint64_t)1 << floor_log_2_d)) {
            // This power works
            more = (uint8_t)floor_log_2_d;
        } else {
            // We have to use the general 65-bit algorithm.  We need to compute
            // (2**power) / d. However, we already have (2**(power-1))/d and
            // its remainder. By doubling both, and then correcting the
            // remainder, we can compute the larger division.
            // don't care about overflow here - in fact, we expect it
            proposed_m += proposed_m;
            const uint64_t twice_rem = rem + rem;
            if (twice_rem >= d || twice_rem < rem)
                proposed_m += 1;
            more = (uint8_t)(floor_log_2_d | _ADD_MARKER);
        }
        result.magic = 1 + proposed_m;
        result.more = more;
        // result.more's shift should in general be ceil_log_2_d. But if we
        // used the smaller power, we subtract one from the shift because we're
        // using the smaller power. If we're using the larger power, we
        // subtract one from the shift because it's taken care of by the add
        // indicator. So floor_log_2_d happens to be correct in both cases,
        // which is why we do it outside of the if statement.
    }
    return result;
}

// d > 1
static inline _udiv64_t _udiv64_gen(uint64_t d) {
    _udiv64_t tmp = _udiv64_gen_internal_(d, 1);
    _udiv64_t ret = {tmp.magic, (uint8_t)(tmp.more & _U64_SHIFT_MASK)};
    return ret;
}

static inline uint64_t _udiv64by64_q_preinv(uint64_t numer, const _udiv64_t* denom) {
    uint64_t q = _umul64to64hi_(numer, denom->magic);
    uint64_t t = ((numer - q) >> 1) + q;
    return t >> denom->more;
}

#endif // __LMMP_LONGLONG_H__

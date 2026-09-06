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

#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/longlong.h"

/* from https://arxiv.org/abs/2204.04342 */

static const uchar binv_tab[128] = {
    1,   171, 205, 183, 57,  163, 197, 239, 241, 27,  61,  167, 41,  19,  53,  223, 225, 139, 173, 151, 25,  131,
    165, 207, 209, 251, 29,  135, 9,   243, 21,  191, 193, 107, 141, 119, 249, 99,  133, 175, 177, 219, 253, 103,
    233, 211, 245, 159, 161, 75,  109, 87,  217, 67,  101, 143, 145, 187, 221, 71,  201, 179, 213, 127, 129, 43,
    77,  55,  185, 35,  69,  111, 113, 155, 189, 39,  169, 147, 181, 95,  97,  11,  45,  23,  153, 3,   37,  79,
    81,  123, 157, 7,   137, 115, 149, 63,  65,  235, 13,  247, 121, 227, 5,   47,  49,  91,  125, 231, 105, 83,
    117, 31,  33,  203, 237, 215, 89,  195, 229, 15,  17,  59,  93,  199, 73,  51,  85,  255};

uint lmmp_binvert_uint_(uint a) {
    lmmp_param_assert(a % 2 == 1);
    uint r, y;

    r = binv_tab[(a / 2) & 0x7F]; /* 8 bits */
    y = 1 - a * r;
    r = r * (1 + y); /* 16 bits */
    y *= y;
    r = r * (1 + y); /* 32 bits */
    return r;
}

ulong lmmp_binvert_ulong_(ulong a) {
    lmmp_param_assert(a % 2 == 1);
    ulong r, y;

    r = binv_tab[(a / 2) & 0x7F]; /* 8 bits */
    y = 1 - a * r;
    r = r * (1 + y); /* 16 bits */
    y *= y;
    r = r * (1 + y); /* 32 bits */
    y *= y;
    r = r * (1 + y); /* 64 bits */
    return r;
}

void lmmp_binvert_2_(mp_ptr dst, mp_srcptr numa) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(numa[0] % 2 == 1);
    mp_limb_t k, t;
    mp_limb_t a1 = numa[1];
    mp_limb_t a0 = numa[0];
    mp_limb_t xn = lmmp_binvert_ulong_(a0);
    mp_limb_t z;
    /*
     xn * a0 == 1 + k * B
     yn := xn * (2 - a * xn) mod B^2
        := xn * (2 - a0 * xn - a1 * xn * B) mod B^2
        := xn * (2 - 1 - k*B - a1 * xn * B) mod B^2
        := xn * (1 - k*B - a1 * xn * B) mod B^2
        := (xn - xn*k * B - a1 * xn^2 * B) mod B^2
    */
    _umul64to128_(a0, xn, &t, &k);
    z = xn * k;
    z += a1 * xn * xn;
    dst[0] = xn;
    dst[1] = -z;
}

static inline void _umul128to192_(uint64_t a_high, uint64_t a_low, uint64_t b_high, uint64_t b_low, uint64_t rr[3]) {
    /*
        | res0 | res1 | res2 |
        |  p0l |  p0h |      |
               |  p1l |  p1h |
               |  p2l |  p2h |
               |      |  p3l |
    */
#if defined(__SIZEOF_INT128__)
    // 128 位累加让进位自然落在寄存器对高位，编译器生成 add/adc 链；
    // rr[2] 为 192 位截断结果，hi 的进一步进位按定义舍弃
    __uint128_t p0 = (__uint128_t)a_low * b_low;
    __uint128_t p1 = (__uint128_t)a_low * b_high;
    __uint128_t p2 = (__uint128_t)a_high * b_low;
    __uint128_t mid = (p0 >> 64) + (uint64_t)p1 + (uint64_t)p2;  // mid>>64 ∈ {0,1,2}
    __uint128_t hi = (p1 >> 64) + (p2 >> 64) + a_high * b_high + (uint64_t)(mid >> 64);
    rr[0] = (uint64_t)p0;
    rr[1] = (uint64_t)mid;
    rr[2] = (uint64_t)hi;
#else
    uint64_t p1_low, p1_high;  // p1 = a_low × b_high
    uint64_t p2_low, p2_high;  // p2 = a_high × b_low
    _umul64to128_(a_low, b_low, rr, rr + 1);
    _umul64to128_(a_low, b_high, &p1_low, &p1_high);
    _umul64to128_(a_high, b_low, &p2_low, &p2_high);
    rr[1] += p1_low;
    uint64_t carry = (rr[1] < p1_low) ? 1 : 0;
    rr[1] += p2_low;
    carry += (rr[1] < p2_low) ? 1 : 0;

    rr[2] = a_high * b_high;
    rr[2] += carry;
    rr[2] += p1_high;
    rr[2] += p2_high;
#endif
}

void lmmp_binvert_3_(mp_ptr restrict dst, mp_srcptr restrict numa) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(numa[0] % 2 == 1);
    /*
           a == a0 + a1 * B^2
     xn * a0 == 1 + k * B^2
     yn := xn * (2 - a * xn) mod B^4
        := xn * (2 - a0 * xn - a1 * xn * B^2) mod B^4
        := xn * (2 - 1 - k*B^2 - a1 * xn * B^2) mod B^4
        := xn * (1 - k*B^2 - a1 * xn * B^2) mod B^4
        := (xn - xn*k * B^2 - a1 * xn^2 * B^2) mod B^4
    */
    lmmp_binvert_2_(dst, numa);
    mp_limb_t k[3];
    mp_limb_t z;
    mp_limb_t a2 = numa[2];
    _umul128to192_(dst[1], dst[0], numa[1], numa[0], k);
    lmmp_debug_assert(k[1] == 0 && k[0] == 1);
#define xn (dst[0])
#define k (k[2])
    z = xn * k;
    z += a2 * xn * xn;
    dst[2] = -z;
#undef xn
#undef k
}

void lmmp_binvert_4_(mp_ptr restrict dst, mp_srcptr restrict numa) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(numa[0] % 2 == 1);
    /*
           a == a0 + a1 * B^2
     xn * a0 == 1 + k * B^2
     yn := xn * (2 - a * xn) mod B^4
        := xn * (2 - a0 * xn - a1 * xn * B^2) mod B^4
        := xn * (2 - 1 - k*B^2 - a1 * xn * B^2) mod B^4
        := xn * (1 - k*B^2 - a1 * xn * B^2) mod B^4
        := (xn - xn*k * B^2 - a1 * xn^2 * B^2) mod B^4
    */
    lmmp_binvert_2_(dst, numa);
    mp_limb_t k[4];
    _umul128to256_(dst[1], dst[0], numa[1], numa[0], k);
    lmmp_debug_assert(k[1] == 0 && k[0] == 1);

#define xn (dst)
#define k (k + 2)
    u128 z = _umul128to128_(k[1], k[0], xn[1], xn[0]);

    mp_limb_t t0, t1;
    _umul64to128_(xn[0], xn[0], &t0, &t1);
    t1 += (xn[1] * xn[0]) << 1;
    z += _umul128to128_(t1, t0, numa[3], numa[2]);

    // dst[2..3] = -z (mod 2^128)
    _u128store(dst + 2, (u128)0 - z);

#undef xn
#undef k
}

/*
unbalanced:
    a := [numa,na]
    a^-1 = a^-1 mod B^na

    p = -k * a^-1 mod B^na
    k = (k + a*p) / B^na
*/

void lmmp_binvert_unbalanced_1_(mp_ptr restrict dst, mp_limb_t a, mp_size_t n) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(a % 2 == 1);
    lmmp_param_assert(n > 1);
    ulong a_binvert = lmmp_binvert_ulong_(a);
    dst[0] = a_binvert;
    mp_size_t i = 0;
    ulong k, p, lo, hi, carry;
    _umul64to128_(a_binvert, a, &lo, &k);
    a_binvert = -a_binvert;
    for (; i < n - 2; i++) {
        p = a_binvert * k;
        dst[i + 1] = p;
        _umul64to128_(p, a, &lo, &hi);
        carry = (k + lo) < lo ? 1 : 0;
        k = hi + carry;
    }
    p = a_binvert * k;
    dst[n - 1] = p;
}

void lmmp_binvert_unbalanced_2_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t n) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(numa[0] % 2 == 1);
    lmmp_param_assert(n > 2);
    mp_limb_t a_binvert[2];
    lmmp_binvert_2_(a_binvert, numa);
    dst[0] = a_binvert[0];
    dst[1] = a_binvert[1];

    mp_limb_t k[4];
    mp_limb_t t[4];
    _umul128to256_(a_binvert[1], a_binvert[0], numa[1], numa[0], k);

    // 此处本应按位取反再加一，得到相反数，但是a_binvert[0]不可能为0，所以进位必定为0
    lmmp_debug_assert(a_binvert[0] != 0);
    a_binvert[0] = ~a_binvert[0];
    a_binvert[1] = ~a_binvert[1];
    a_binvert[0]++;

    mp_size_t i = 0;
    if (n % 2 == 0) {
        for (; i < n - 4; i += 2) {
            u128 p = _umul128to128_(a_binvert[1], a_binvert[0], k[3], k[2]);
            dst[i + 2] = _u128low(p);
            dst[i + 3] = _u128high(p);
            _umul128to256_(_u128high(p), _u128low(p), numa[1], numa[0], t);
            // t = a*p；k = (k + t) >> 128，结果存回 k[2..3]
            u128 klow = _u128load(k + 2);
            u128 s = _u128load(t) + klow;
            uint c = (uint)(s < klow);
            _u128store(k + 2, _u128load(t + 2) + c);
        }
        u128 p = _umul128to128_(a_binvert[1], a_binvert[0], k[3], k[2]);
        dst[i + 2] = _u128low(p);
        dst[i + 3] = _u128high(p);
    } else {
        for (; i < n - 3; i += 2) {
            u128 p = _umul128to128_(a_binvert[1], a_binvert[0], k[3], k[2]);
            dst[i + 2] = _u128low(p);
            dst[i + 3] = _u128high(p);
            _umul128to256_(_u128high(p), _u128low(p), numa[1], numa[0], t);
            // t = a*p；k = (k + t) >> 128，结果存回 k[2..3]
            u128 klow = _u128load(k + 2);
            u128 s = _u128load(t) + klow;
            uint c = (uint)(s < klow);
            _u128store(k + 2, _u128load(t + 2) + c);
        }
        u128 p = _umul128to128_(a_binvert[1], a_binvert[0], k[3], k[2]);
        dst[i + 2] = _u128low(p);
    }
}

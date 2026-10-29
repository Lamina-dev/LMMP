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

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/powmod.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

#define getbit(p, bi) ((p[(bi - 1) / LIMB_BITS] >> (bi - 1) % LIMB_BITS) & 1)

static inline mp_limb_t getbits(const mp_limb_t* p, mp_bitcnt_t bi, mp_bitcnt_t nbits) {
    mp_bitcnt_t nbits_in_r;
    mp_limb_t r;
    mp_size_t i;

    if (bi <= nbits) {
        return p[0] & (((mp_limb_t)1 << bi) - 1);
    } else {
        bi -= nbits;                     /* bit index of low bit to extract */
        i = bi / LIMB_BITS;              /* word index of low bit to extract */
        bi %= LIMB_BITS;                 /* bit index in low word */
        r = p[i] >> bi;                  /* extract (low) bits */
        nbits_in_r = LIMB_BITS - bi;     /* number of bits now in r */
        if (nbits_in_r < nbits)          /* did we get enough bits? */
            r += p[i + 1] << nbits_in_r; /* prepend bits from higher word */
        return r & (((mp_limb_t)1 << nbits) - 1);
    }
}

static inline mp_bitcnt_t count_bits(mp_srcptr p, mp_size_t n) {
    return (n - 1) * LIMB_BITS + lmmp_limb_bits_(p[n - 1]);
}

/* mul_128/sqr_128/mont2_redc 与 is_prime_2.c 同源拷贝（编译器分派的
   惯例与正确性论证见彼处） */
static inline void mul_128(u128 a, u128 b, u128* hi, u128* lo) {
#if defined(__clang__) && !defined(LMMP_ASM_ARM64)
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
#else
    ulong u[4];
    _umul128to256_(_u128high(a), _u128low(a), _u128high(b), _u128low(b), u);
    *lo = ((u128)u[1] << 64) | u[0];
    *hi = ((u128)u[3] << 64) | u[2];
#endif
}

/* a^2 -> (hi, lo)：交叉积单次乘法后整体加倍（3 乘法平方） */
static inline void sqr_128(u128 a, u128* hi, u128* lo) {
#if defined(__clang__) && !defined(LMMP_ASM_ARM64)
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
#else
    ulong u[4];
    _usqr128to256_(_u128high(a), _u128low(a), u);
    *lo = ((u128)u[1] << 64) | u[0];
    *hi = ((u128)u[3] << 64) | u[2];
#endif
}

/* 2 limb REDC：r = (T + q*n)/B^2，T = thi*B^2 + tlo < n*B^2，q = tlo*ninv
   mod B^2（u128 截断乘恰为其低 128 位）。n 接近 B^2 时和可达 129 位，
   回绕进位下按模 2^128 回绕减 n 恰为真值且已 < n */
static inline u128 mont2_redc(u128 thi, u128 tlo, u128 n, u128 ninv) {
#if defined(LMMP_ASM_ARM64)
    u128 q = tlo * ninv;
    u128 qh, ql;
    mul_128(q, n, &qh, &ql);
    ulong t0 = _u128low(tlo), t1 = _u128high(tlo);
    ulong p0 = _u128low(ql), p1 = _u128high(ql);
    ulong k0 = (t0 + p0) < t0; /* lo 低 limb 进位（lo 值本身无消费者） */
    ulong l1 = t1 + p1;
    ulong k1 = l1 < t1;
    l1 += k0;
    k1 |= l1 < k0;
    ulong h0 = _u128low(thi) + _u128low(qh);
    ulong j0 = h0 < _u128low(qh);
    h0 += k1;
    j0 |= h0 < k1;
    ulong h1 = _u128high(thi) + _u128high(qh);
    ulong j1 = h1 < _u128high(qh);
    h1 += j0;
    j1 |= h1 < j0;
    ulong n0 = _u128low(n), n1 = _u128high(n);
    if (j1 || (h1 > n1) || (h1 == n1 && h0 >= n0)) {
        ulong bw = h0 < n0; /* 借位跨 limb 传播 */
        h0 -= n0;
        h1 -= n1 + bw; /* j1 分支下回绕即真值 */
    }
    return ((u128)h1 << 64) | h0;
#else
    u128 q = tlo * ninv;
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
#endif
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

void lmmp_powmod_2_(
    mp_ptr    restrict dst,
    mp_srcptr restrict  bp,
    mp_srcptr restrict  ep,
    mp_size_t           en,
    mp_srcptr restrict mod
) {
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL && mod != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);
    lmmp_param_assert(mod[0] % 2 == 1);
    lmmp_param_assert(mod[1] != 0);

    u128 n = _u128load(mod);
    if (_u128load(bp) == 0) {
        lmmp_zero(dst, 2);
        return;
    }
    mp_bitcnt_t ebi = count_bits(ep, en);
    /* 指数为 1：契约 [bp,2] < [mod,2]，直接拷贝即已取模 */
    if (ebi == 1) {
        lmmp_copy(dst, bp, 2);
        return;
    }

    unsigned windowsize = lmmp_powmod_win_size_(ebi);
    lmmp_debug_assert(windowsize < ebi);
    /* ninv = -m^(-1) mod B^2：u128 回绕取负 */
    mp_limb_t inv[2];
    lmmp_binvert_2_(inv, mod);
    u128 ninv = 0 - _u128load(inv);

    /* 进蒙域：x_m = b*B^2 mod m，一次 4/2 除法（低位补两个零 limb 实现 <<B^2） */
    u128 u;
    {
        mp_limb_t num[4] = {0, 0, bp[0], bp[1]};
        mp_limb_t r[2];
        lmmp_div_(NULL, r, num, 4, mod, 2);
        u = _u128load(r);
    }

    mp_bitcnt_t cnt;
    unsigned this_windowsize;
    mp_limb_t expbits;
    TEMP_DECL;
    u128* restrict pp = TALLOC_TYPE((mp_size_t)1 << (windowsize - 1), u128);

    pp[0] = u;
    if (windowsize > 1) {
        u128 b2 = mont2_sqr(u, n, ninv);
        for (mp_size_t i = 1; i < ((mp_size_t)1 << (windowsize - 1)); i++)
            pp[i] = mont2_mul_2(pp[i - 1], b2, n, ninv);

        expbits = getbits(ep, ebi, windowsize);
        ebi -= windowsize;

        ctz_shr_u64(expbits, expbits, cnt);
        ebi += cnt;

        u = pp[expbits >> 1];
    } else {
        --ebi;
    }

    while (ebi != 0) {
        while (getbit(ep, ebi) == 0) {
            u = mont2_sqr(u, n, ninv);
            if (--ebi == 0)
                goto done;
        }

        /* The next bit of the exponent is 1.  Now extract the largest block of
           bits <= windowsize, and such that the least significant bit is 1.  */

        expbits = getbits(ep, ebi, windowsize);
        this_windowsize = LMMP_MIN(windowsize, ebi);

        ctz_shr_u64(expbits, expbits, cnt);
        this_windowsize -= cnt;
        ebi -= this_windowsize;

        while (this_windowsize > 1) {
            u = mont2_sqr(u, n, ninv);
            u = mont2_sqr(u, n, ninv);
            this_windowsize -= 2;
        }

        if (this_windowsize != 0)
            u = mont2_sqr(u, n, ninv);
        u = mont2_mul_2(u, pp[expbits >> 1], n, ninv);
    }

done:
    /* 出蒙域：REDC(u * 1) = u*B^(-2) mod m */
    _u128store(dst, mont2_redc(0, u, n, ninv));
    TEMP_FREE;
}

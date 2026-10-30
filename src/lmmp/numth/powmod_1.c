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

/* 1 limb REDC：r = (T + k*m)/B mod m，k = T_lo*ninv mod B，ninv = -m^(-1) mod B。
   T < m*B 时结果 < 2m；k*m + T < 2^129，第 129 位 c 由 128 位回绕捕获，
   回绕进位下 hi - m 即规范结果 */
static inline mp_limb_t powmod1_redc_(u128 t, mp_limb_t m, mp_limb_t ninv) {
    mp_limb_t k = _u128low(t) * ninv;
    u128 km = (u128)k * m;
    u128 sum = km + t;
    uint c = sum < km;
    mp_limb_t hi = _u128high(sum);
    if (c)
        return hi - m;
    return hi >= m ? hi - m : hi;
}

void lmmp_powmod_1_(
    mp_ptr    restrict dst,
    mp_srcptr restrict  bp,
    mp_srcptr restrict  ep,
    mp_size_t           en,
    mp_limb_t          mod
) {
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);
    lmmp_param_assert(mod % 2 == 1 && mod > 1);
    lmmp_param_assert(bp[0] < mod);

    if (bp[0] == 0) {
        dst[0] = 0;
        return;
    }
    mp_bitcnt_t ebi = count_bits(ep, en);
    /* 指数为 1：契约 [bp,1] < mod，直接拷贝即已取模 */
    if (ebi == 1) {
        dst[0] = bp[0];
        return;
    }

    unsigned windowsize = lmmp_powmod_win_size_(ebi);
    lmmp_debug_assert(windowsize < ebi);
    mp_limb_t ninv = 0 - lmmp_binvert_ulong_(mod);
    /* 进蒙域：x_m = b*B mod m，128/64 一步除法（b < mod 保证商不溢出） */
    mp_limb_t u;
    (void)_udiv128by64to64_(bp[0], 0, mod, &u);

    mp_bitcnt_t cnt;
    unsigned this_windowsize;
    mp_limb_t expbits;
    TEMP_DECL;
    mp_ptr restrict pp = TALLOC_TYPE((mp_size_t)1 << (windowsize - 1), mp_limb_t);

    pp[0] = u;
    if (windowsize > 1) {
        mp_limb_t b2 = powmod1_redc_((u128)u * u, mod, ninv);
        for (mp_size_t i = 1; i < ((mp_size_t)1 << (windowsize - 1)); i++)
            pp[i] = powmod1_redc_((u128)pp[i - 1] * b2, mod, ninv);

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
            u = powmod1_redc_((u128)u * u, mod, ninv);
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
            u = powmod1_redc_((u128)u * u, mod, ninv);
            u = powmod1_redc_((u128)u * u, mod, ninv);
            this_windowsize -= 2;
        }

        if (this_windowsize != 0)
            u = powmod1_redc_((u128)u * u, mod, ninv);
        u = powmod1_redc_((u128)u * pp[expbits >> 1], mod, ninv);
    }

done:
    /* 出蒙域：REDC(u * 1) = u*B^(-1) mod m */
    dst[0] = powmod1_redc_(u, mod, ninv);
    TEMP_FREE;
}

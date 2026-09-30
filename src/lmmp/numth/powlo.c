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

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


static inline void lmmp_sqrlo_n_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_size_t             n,
    mp_ptr    restrict   tp
) {
    if (n < MULLO_DC_THRESHOLD) {
        lmmp_sqrlo_dc_(dst, numa, tp, n);
    } else {
        lmmp_mullo_fft_(dst, numa, numa, n, tp);
    }
}

static inline void lmmp_mullo_n_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_srcptr restrict numb,
    mp_size_t             n,
    mp_ptr    restrict   tp
) {
    if (n < MULLO_DC_THRESHOLD) {
        lmmp_mullo_dc_(dst, numa, numb, tp, n);
    } else {
        lmmp_mullo_fft_(dst, numa, numb, n, tp);
    }
}

static inline mp_size_t win_size(mp_size_t eb) {
    mp_size_t k;
    static mp_bitcnt_t x[] = {7, 25, 81, 241, 673, 1793, 4609, 11521, 28161, ~(mp_bitcnt_t)0};
    for (k = 0; eb > x[k++];);
    return k;
}

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

void lmmp_powlo_(mp_ptr restrict dst, mp_srcptr restrict bp, mp_size_t n, mp_srcptr restrict ep, mp_size_t en) {
    lmmp_param_assert(ep[en - 1] > 0);
    lmmp_param_assert(n > 0 && en > 0);
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL);
    if (n == 1) {
        lmmp_powlo_1_(dst, bp, ep, en);
        return;
    }
    if (n == 2) {
        lmmp_powlo_2_(dst, bp, ep, en);
        return;
    }
    mp_bitcnt_t cnt, ebi;
    unsigned windowsize, this_windowsize;
    mp_limb_t expbits;
    mp_limb_t* pp;
    long i;
    int flipflop;
    TEMP_DECL;
    mp_ptr restrict tp = TALLOC_TYPE(5 * n, mp_limb_t);
    mp_ptr restrict scratch = tp + 3 * n;

    ebi = count_bits(ep, en);

    /* 指数为 1：[bp,n] mod B^n 即截断本身，免去建表与梯子 */
    if (ebi == 1) {
        lmmp_copy(dst, bp, n);
        TEMP_FREE;
        return;
    }

    windowsize = win_size(ebi);
    if (windowsize > 1) {
        mp_limb_t *this_pp, *last_pp;
        lmmp_debug_assert(windowsize < ebi);

        pp = TALLOC_TYPE((n << (windowsize - 1)), mp_limb_t);

        this_pp = pp;

        lmmp_copy(this_pp, bp, n);

        /* Store b^2 in tp.  */
        lmmp_sqrlo_n_(tp, bp, n, scratch);

        /* Precompute odd powers of b and put them in the temporary area at pp.  */
        i = (1 << (windowsize - 1)) - 1;
        do {
            last_pp = this_pp;
            this_pp += n;
            lmmp_mullo_n_(this_pp, last_pp, tp, n, scratch);
        } while (--i != 0);

        expbits = getbits(ep, ebi, windowsize);
        ebi -= windowsize;

        ctz_shr_u64(expbits, expbits, cnt);
        ebi += cnt;

        lmmp_copy(dst, pp + n * (expbits >> 1), n);
    } else {
        pp = tp + n;
        lmmp_copy(pp, bp, n);
        lmmp_copy(dst, bp, n);
        --ebi;
    }

    flipflop = 0;

    do {
        while (getbit(ep, ebi) == 0) {
            lmmp_sqrlo_n_(tp, dst, n, scratch);
            LMMP_SWAP(dst, tp, mp_ptr);
            flipflop = !flipflop;
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
            lmmp_sqrlo_n_(tp, dst, n, scratch);
            lmmp_sqrlo_n_(dst, tp, n, scratch);
            this_windowsize -= 2;
        }

        if (this_windowsize != 0)
            lmmp_sqrlo_n_(tp, dst, n, scratch);
        else {
            LMMP_SWAP(dst, tp, mp_ptr);
            flipflop = !flipflop;
        }
        lmmp_mullo_n_(dst, tp, pp + n * (expbits >> 1), n, scratch);
    } while (ebi != 0);

done:
    if (flipflop)
        lmmp_copy(tp, dst, n);
    TEMP_FREE;
}

/*
    模 B 与模 B^2 的特化梯子。

    模 B^n 的乘法无需归约（截断低位即取模），小 n 时全积与窗口表退化为
    标量运算：1 limb 为 u64 自然回绕乘；2 limb 借 longlong.h 的 128 位
    低位积内联 _umul128to128_。梯子结构与泛型版一致（滑窗 + 首窗口），
    仅指数为 1 时提前截断返回。
*/

void lmmp_powlo_1_(mp_ptr restrict dst, mp_srcptr restrict bp, mp_srcptr restrict ep, mp_size_t en) {
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);
    mp_limb_t u = bp[0];
    mp_bitcnt_t ebi = count_bits(ep, en);

    /* 指数为 1：b mod B 即 [bp,1] 本身 */
    if (ebi == 1) {
        dst[0] = u;
        return;
    }

    unsigned windowsize = win_size(ebi);
    lmmp_debug_assert(windowsize < ebi);
    mp_bitcnt_t cnt;
    unsigned this_windowsize;
    mp_limb_t expbits;
    TEMP_DECL;
    mp_ptr restrict pp = TALLOC_TYPE((mp_size_t)1 << (windowsize - 1), mp_limb_t);

    pp[0] = u;
    if (windowsize > 1) {
        mp_limb_t b2 = u * u; /* b^2 mod B：u64 回绕乘 */
        for (mp_size_t i = 1; i < ((mp_size_t)1 << (windowsize - 1)); i++)
            pp[i] = pp[i - 1] * b2;

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
            u *= u;
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
            u *= u;
            u *= u;
            this_windowsize -= 2;
        }

        if (this_windowsize != 0)
            u *= u;
        u *= pp[expbits >> 1];
    }

done:
    dst[0] = u;
    TEMP_FREE;
}

void lmmp_powlo_2_(mp_ptr restrict dst, mp_srcptr restrict bp, mp_srcptr restrict ep, mp_size_t en) {
    lmmp_param_assert(dst != NULL && bp != NULL && ep != NULL);
    lmmp_param_assert(en > 0 && ep[en - 1] > 0);
    u128 u = _u128load(bp);
    mp_bitcnt_t ebi = count_bits(ep, en);

    /* 指数为 1：b mod B^2 即 [bp,2] 本身 */
    if (ebi == 1) {
        _u128store(dst, u);
        return;
    }

    unsigned windowsize = win_size(ebi);
    lmmp_debug_assert(windowsize < ebi);
    mp_bitcnt_t cnt;
    unsigned this_windowsize;
    mp_limb_t expbits;
    TEMP_DECL;
    u128* restrict pp = TALLOC_TYPE((mp_size_t)1 << (windowsize - 1), u128);

    pp[0] = u;
    if (windowsize > 1) {
        u128 b2 = _umul128to128_(_u128high(u), _u128low(u), _u128high(u), _u128low(u)); /* b^2 mod B^2 */
        for (mp_size_t i = 1; i < ((mp_size_t)1 << (windowsize - 1)); i++)
            pp[i] = _umul128to128_(_u128high(pp[i - 1]), _u128low(pp[i - 1]), _u128high(b2), _u128low(b2));

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
            u = _umul128to128_(_u128high(u), _u128low(u), _u128high(u), _u128low(u));
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
            u = _umul128to128_(_u128high(u), _u128low(u), _u128high(u), _u128low(u));
            u = _umul128to128_(_u128high(u), _u128low(u), _u128high(u), _u128low(u));
            this_windowsize -= 2;
        }

        if (this_windowsize != 0)
            u = _umul128to128_(_u128high(u), _u128low(u), _u128high(u), _u128low(u));
        u = _umul128to128_(_u128high(u), _u128low(u), _u128high(pp[expbits >> 1]), _u128low(pp[expbits >> 1]));
    }

done:
    _u128store(dst, u);
    TEMP_FREE;
}

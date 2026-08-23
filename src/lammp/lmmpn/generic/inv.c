/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../../include/lammp/impl/longlong.h"
#include "../../../../include/lammp/lmmpn.h"


mp_limb_t lmmp_inv_2_1_(mp_limb_t xh, mp_limb_t xl) {
    mp_limb_t r, m;
    {
        mp_limb_t p, ql;
        unsigned ul, uh, qh;

        /* For notation, let b denote the half-limb base, so that B = b^2.
           Split u1 = b uh + ul. */
        ul = xh & LLIMB_MASK;
        uh = xh >> (LIMB_BITS / 2);

        /* Approximation of the high half of quotient. Differs from the 2/1
           inverse of the half limb uh, since we have already subtracted
           u0. */
        qh = (xh ^ LIMB_MAX) / uh;

        /* Adjust to get a half-limb 3/2 inverse, i.e., we want

           qh' = floor( (b^3 - 1) / u) - b = floor ((b^3 - b u - 1) / u
        = floor( (b (~u) + b-1) / u),

           and the remainder

           r = b (~u) + b-1 - qh (b uh + ul)
           = b (~u - qh uh) + b-1 - qh ul

           Subtraction of qh ul may underflow, which implies adjustments.
           But by normalization, 2 u >= B > qh ul, so we need to adjust by
           at most 2.
        */

        r = ((~xh - (mp_limb_t)qh * uh) << (LIMB_BITS / 2)) | LLIMB_MASK;

        p = (mp_limb_t)qh * ul;
        /* Adjustment steps taken from udiv_qrnnd_c */
        if (r < p) {
            qh--;
            r += xh;
            if (r >= xh) /* i.e. we didn't get carry when adding to r */
                if (r < p) {
                    qh--;
                    r += xh;
                }
        }
        r -= p;

        /* Low half of the quotient is

           ql = floor ( (b r + b-1) / u1).

           This is a 3/2 division (on half-limbs), for which qh is a
           suitable inverse. */

        p = (r >> (LIMB_BITS / 2)) * qh + r;
        /* Unlike full-limb 3/2, we can add 1 without overflow. For this to
           work, it is essential that ql is a full mp_limb_t. */
        ql = (p >> (LIMB_BITS / 2)) + 1;

        /* By the 3/2 trick, we don't need the high half limb. */
        r = (r << (LIMB_BITS / 2)) + LLIMB_MASK - ql * xh;

        if (r >= (LIMB_MAX & (p << (LIMB_BITS / 2)))) {
            ql--;
            r += xh;
        }
        m = ((mp_limb_t)qh << (LIMB_BITS / 2)) + ql;
        if (r >= xh) {
            m++;
            r -= xh;
        }
    }

    /* Now m is the 2/1 inverse of u1. If u0 > 0, adjust it to become a
       3/2 inverse. */
    if (xl > 0) {
        mp_limb_t th, tl;
        r = ~r;
        r += xl;
        if (r < xl) {
            m--;
            if (r >= xh) {
                m--;
                r -= xh;
            }
            r -= xh;
        }
        _umul64to128_(xl, m, &tl, &th);
        r += th;
        if (r < th) {
            m--;
            m -= ((r > xh) | ((r == xh) & (tl > xl)));
        }
    }

    return m;
}

mp_limb_t lmmp_inv_1_(mp_limb_t x) {
    mp_limb_t r, m;
    {
        mp_limb_t p, ql;
        unsigned ul, uh, qh;

        ul = x & LLIMB_MASK;
        uh = x >> (LIMB_BITS / 2);
        qh = (x ^ LIMB_MAX) / uh;

        r = ((~x - (mp_limb_t)qh * uh) << (LIMB_BITS / 2)) | LLIMB_MASK;
        p = (mp_limb_t)qh * ul;
        if (r < p) {
            qh--;
            r += x;
            if (r >= x)
                if (r < p) {
                    qh--;
                    r += x;
                }
        }
        r -= p;
        p = (r >> (LIMB_BITS / 2)) * qh + r;
        ql = (p >> (LIMB_BITS / 2)) + 1;
        r = (r << (LIMB_BITS / 2)) + LLIMB_MASK - ql * x;
        if (r >= (LIMB_MAX & (p << (LIMB_BITS / 2)))) {
            ql--;
            r += x;
        }
        m = ((mp_limb_t)qh << (LIMB_BITS / 2)) + ql;
        if (r >= x) {
            m++;
            r -= x;
        }
    }
    return m;
}

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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


void lmmp_inv_basecase_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa[na - 1] >= LIMB_B_2);
    if (na == 1)
        *dst = lmmp_inv_1_(*numa);
    else {
        TEMP_DECL;
        mp_ptr restrict xp = TALLOC_TYPE(2 * na, mp_limb_t);
        mp_size_t i = na;
        do {
            xp[--i] = LIMB_MAX;
        } while (i);
        lmmp_not_(xp + na, numa, na);
        //[xp,2*na] = B^(2*na)-1 - [numa,na]*B^na

        if (na == 2) {
            lmmp_div_2_s_(dst, xp, 4, numa);
        } else {
            mp_limb_t inv21 = lmmp_inv_2_1_(numa[na - 1], numa[na - 2]);
            if (na < DIV_DIVIDE_THRESHOLD) {
                lmmp_div_basecase_(dst, xp, 2 * na, numa, na, inv21);
            } else {
                lmmp_div_divide_(dst, xp, 2 * na, numa, na, inv21);
            }
        }
        TEMP_FREE;
    }
}

void lmmp_invappr_newton_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na > 4);
    lmmp_param_assert(numa[na - 1] >= LIMB_B_2);
    
    mp_limb_t cy;
    mp_size_t nr = na, mn;
    mp_size_t sizes[LIMB_BITS], *sizp = sizes;

    do {
        *sizp = nr;
        nr = (nr >> 1) + 1;
        ++sizp;
    } while (nr >= INV_NEWTON_THRESHOLD);

    numa += na;
    dst += na;

    lmmp_inv_basecase_(dst - nr, numa - nr, nr);

    TEMP_DECL;
    mp_ptr restrict xp = TALLOC_TYPE(3 * (na >> 1) + 3, mp_limb_t);
    do {
        na = *--sizp;

        // ar = 0:[numa-nr,nr]
        // an = 0:[numa-na,na]
        // ir = 1:[dst-nr,nr] = (B^(2*nr)-1)/ar - [0|1]
        // rem = ir*an-B^(na+nr)
        //-2*B^na < rem < 2*B^na

        //[xp] = rem
        if (na < INV_MODM_THRESHOLD || (mn = lmmp_fft_next_size_(na + 1)) >= na + nr) {
            lmmp_mul_(xp, numa - na, na, dst - nr, nr);
            lmmp_add_n_(xp + nr, xp + nr, numa - na, na + 1 - nr);
            cy = 1;  // for mod B^(na+1)
        } else {     // nr < na < mn < na+nr

            //[xp,mn] = [dst,nr] * [numa,na] mod (B^mn-1)
            lmmp_mul_mersenne_(xp, mn, numa - na, na, dst - nr, nr);

            //[xp,mn] += [numa,na]*B^nr mod (B^mn-1)
            cy = lmmp_add_n_(xp + nr, xp + nr, numa - na, mn - nr);
            cy = lmmp_add_nc_(xp, xp, numa - (na - (mn - nr)), na - (mn - nr), cy);

            //[xp,mn] -= B^(na+nr) mod (B^mn-1)
            xp[mn] = 1;
            lmmp_dec_1(xp + na + nr - mn, 1 - cy);
            lmmp_dec_1(xp, 1 - xp[mn]);

            cy = 0;  // for mod (B^mn-1)
        }

        // adjust ir,rem s.t.
        //  -B^na < rem = ir*an - B^(na+nr) < 0
        //  use this we can prove B^nr <= ir < 2*B^nr
        //  so inc/dec ir won't overflow
        if (xp[na] < 2) {  // rem>=0

            // rem-=cy*an s.t. rem[na]=0
            if ((cy = xp[na])) {
                if (!lmmp_sub_n_(xp, xp, numa - na, na)) {
                    ++cy;
                    lmmp_sub_n_(xp, xp, numa - na, na);
                }
            }

            // rem-=cy*an s.t. 0<=rem<an
            if (lmmp_cmp_(xp, numa - na, na) >= 0) {
                lmmp_sub_n_(xp, xp, numa - na, na);
                ++cy;
            }

            // 0 < an-rem <= an < B^na , trunc to nr limbs
            lmmp_sub_nc_(xp + 2 * nr, numa - nr, xp + na - nr, nr, lmmp_cmp_(xp, numa - na, na - nr) > 0);
            ++cy;

            lmmp_dec_1(dst - nr, cy);
        } else {  // rem<0
            if (cy)
                lmmp_dec(xp);  // for neg to not
            // else (neg to not) compensate (mod transfer)

            if (xp[na] != LIMB_MAX) {
                mp_limb_t not_xp = lmmp_add_n_(xp, xp, numa - na, na);
                lmmp_assert(xp[na] + not_xp == LIMB_MAX);
                lmmp_inc(dst - nr);
            }

            //-rem
            lmmp_not_(xp + 2 * nr, xp + na - nr, nr);
        }

        // in = 1:[dst-na,na]
        // in = ir*B^(na-nr) + ir*(-rem/B^(na-nr))/B^(3*nr-na)
        // use inequality an*ir!=B^(na+nr),
        //(otherwise obviously contradictory),
        // we can prove
        //  an*in <= an*ir * ( 2*B^(na+nr) - an*ir ) * B^(-2*nr) < B^(2*na)
        // so in < B^(2*na)/an <= 2*B^(na),
        // inc below won't overflow

        // and via inequality -B^na < an*ir - B^(na+nr) < 0
        // we can prove in = (B^(2*na)-1)/an - [0|1]
        lmmp_mul_n_(xp, xp + 2 * nr, dst - nr, nr);
        cy = lmmp_add_n_(xp + nr, xp + nr, xp + 2 * nr, 2 * nr - na);
        if (lmmp_add_nc_(dst - na, xp + 3 * nr - na, xp + 4 * nr - na, na - nr, cy))
            lmmp_inc(dst - nr);

        nr = na;
    } while (sizp != sizes);
    TEMP_FREE;
}

void lmmp_inv_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa[na - 1] != 0);
    mp_limb_t high = numa[na - 1];
    int nsh = lmmp_leading_zeros_(high);
    TEMP_DECL;
    if (dst == numa || nsh || nf) {
        nf += nsh != 0;
        mp_ptr restrict numa2 = TALLOC_TYPE(na + nf, mp_limb_t);
        lmmp_zero(numa2, nf);
        if (nsh)
            lmmp_shl_(numa2 + nf, numa, na, nsh);
        else
            lmmp_copy(numa2 + nf, numa, na);
        numa = numa2;
    }
    lmmp_invappr_(dst, numa, na + nf);
    if (nsh)
        lmmp_shr_c_(dst, dst, na + nf, LIMB_BITS - nsh, (mp_limb_t)1 << nsh);
    else
        dst[na + nf] = 1;
    TEMP_FREE;
}

void lmmp_invappr_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    if (na < INV_NEWTON_THRESHOLD)
        lmmp_inv_basecase_(dst, numa, na);
    else
        lmmp_invappr_newton_(dst, numa, na);
}

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
#include "../../../include/lmmp/impl/mul_cache.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


void lmmp_inv_prediv_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t ni) {
    lmmp_param_assert(na >= ni);
    lmmp_param_assert(ni > 0);
    lmmp_param_assert(numa[na - 1] >= LIMB_B_2);
    TEMP_DECL;
    mp_limb_t cy;
    mp_ptr restrict tp = TALLOC_TYPE(ni + 1, mp_limb_t);

    if (na == ni) {
        lmmp_copy(tp + 1, numa, ni);
        tp[0] = 1;
        cy = 0;
    } else {
        cy = lmmp_add_1_(tp, numa + na - (ni + 1), ni + 1, 1);
    }
    if (cy)
        lmmp_zero(dst, ni);
    else {
        mp_ptr restrict invappr = TALLOC_TYPE(ni + 1, mp_limb_t);
        lmmp_invappr_(invappr, tp, ni + 1);
        lmmp_copy(dst, invappr + 1, ni);
    }
    TEMP_FREE;
}

mp_limb_t lmmp_div_mulinv_(
    mp_ptr    restrict    dstq,
    mp_ptr    restrict    numa,
    mp_size_t               na,
    mp_srcptr restrict    numb,
    mp_size_t               nb,
    mp_srcptr restrict invappr,
    mp_size_t               ni
) {
    lmmp_param_assert(na >= nb && nb >= ni);
    lmmp_param_assert(ni > 0);
    lmmp_param_assert(numb[nb - 1] >= LIMB_B_2);
    mp_size_t nq = na - nb, ntp = LMMP_MIN(ni, nq) + nb;
    mp_limb_t qh;
    TEMP_DECL;
    mp_ptr restrict tp = TALLOC_TYPE(ntp, mp_limb_t);

    numa += nq;
    dstq += nq;

    qh = lmmp_cmp_(numa, numb, nb) >= 0;
    if (qh) {
        lmmp_sub_n_(numa, numa, numb, nb);
	}

    fft_gr_cache mersenne_ctx;
    int mersenne_flag = 0;
    fft_cache fft_ctx;
    int fft_flag = 0;
    int small_flag = 0;

    while (nq) {
        if (nq < ni) {
            invappr += ni - nq;
            ni = nq;
            small_flag = 1;
        }
        numa -= ni;
        dstq -= ni;
        nq -= ni;

        mp_size_t mn, wn;
        mp_limb_t cy;

        if (small_flag == 1 || ni < MUL_FFT_THRESHOLD)
            lmmp_mul_n_(tp, numa + nb, invappr, ni);
        else {
            if (fft_flag == 0) {
                mp_size_t hn = lmmp_fft_next_size_((2 * ni + 1) / 2);
                lmmp_mul_fft_cache_init_(tp, hn, numa + nb, ni, invappr, ni, &fft_ctx);
                fft_flag = 1;
            } else {
                lmmp_mul_fft_cache_(tp, numa + nb, &fft_ctx);
            }
        }
        cy = lmmp_add_n_(dstq, tp + ni, numa + nb, ni);
        lmmp_assert(cy == 0);

        if (nb < DIV_MULINV_MODM_THRESHOLD || (mn = lmmp_fft_next_size_(nb + 1)) >= nb + ni) {
            lmmp_mul_(tp, numb, nb, dstq, ni);  // nb+ni limbs, high 'ni' cancels
		} else {
            // 0<wn<ni<=nb<mn<nb+ni
            wn = nb + ni - mn;

            // x=b*q
            // tp=x mod 2^mn-1
            if (small_flag == 1)
                lmmp_mul_mersenne_(tp, mn, dstq, ni, numb, nb);
            else {
                if (mersenne_flag == 0) {
                    lmmp_mul_mersenne_cache_init_(tp, mn, dstq, ni, numb, nb, &mersenne_ctx);
                    mersenne_flag = 1;
                } else {
                    lmmp_mul_mersenne_cache_(tp, dstq, &mersenne_ctx);
                }
            }            

            // tp-=ah:0 mod B^mn-1, if result=0, represent it as B^mn-1
            cy = lmmp_sub_nc_(tp, tp, numa + mn, wn, 1);
            if (cy)
                cy = lmmp_sub_1_(tp + wn, tp + wn, mn - wn, 1);
            if (!cy)
                lmmp_inc(tp);

            // if al<<tp,
            if (lmmp_cmp_(numa + nb, tp + nb, mn - nb) < 0) {
                // maybe ah=xh+1 and al<<xl,
                //  so we subtracted 1 too much when tp-=ah,
                //  now tp=xl-1 mod B^mn-1, and 0<=al<<xl-1<B^mn-1, so tp=xl-1
                // or ah=xh and al>=xl,
                //  tp=xl mod B^mn-1, the only possibility is we represented xl=0 as tp=B^mn-1
                // whatever, just inc and then tp=xl
                tp[mn] = 0;  // set a limit
                lmmp_inc(tp);
            }
        }

        mp_limb_t r = numa[nb] - tp[nb];
        cy = lmmp_sub_n_(numa, numa, tp, nb);

        while ((r -= cy) || lmmp_cmp_(numa, numb, nb) >= 0) {
            lmmp_inc(dstq);
            cy = lmmp_sub_n_(numa, numa, numb, nb);
        }
    }

    if (mersenne_flag == 1)
        lmmp_fft_gr_cache_free_(&mersenne_ctx);
    if (fft_flag == 1)
        lmmp_fft_cache_free_(&fft_ctx);
    TEMP_FREE;
    return qh;
}

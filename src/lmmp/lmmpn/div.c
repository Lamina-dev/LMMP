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
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


mp_limb_t lmmp_div_s_(
    mp_ptr    restrict dstq,
    mp_ptr    restrict numa,
    mp_size_t            na,
    mp_srcptr restrict numb,
    mp_size_t            nb
) {
    lmmp_param_assert(na >= nb && nb > 0);
    lmmp_param_assert(numa != NULL && numb != NULL && dstq != NULL);
    lmmp_param_assert(numb[nb - 1] >= LIMB_B_2);

    TEMP_DECL;
    mp_limb_t nq = na - nb;
    mp_limb_t qh;
    if (nq == 0) {
        qh = lmmp_cmp_(numa, numb, nb) >= 0;
        if (qh)
            lmmp_sub_n_(numa, numa, numb, nb);
    } else if (nb == 1) {
        qh = lmmp_div_1_s_(dstq, numa, na, *numb);
    } else if (nb == 2) {
        qh = lmmp_div_2_s_(dstq, numa, na, numb);
    } else if (nq < nb) {
        qh = lmmp_div_s_(dstq, numa + na - 2 * nq, 2 * nq, numb + nb - nq, nq);

        mp_ptr restrict tp = TALLOC_TYPE(nb, mp_limb_t);
        if (nq > nb - nq)
            lmmp_mul_(tp, dstq, nq, numb, nb - nq);
        else
            lmmp_mul_(tp, numb, nb - nq, dstq, nq);

        mp_limb_t cy = lmmp_sub_n_(numa, numa, tp, nb);
        if (qh)
            cy += lmmp_sub_n_(numa + nq, numa + nq, numb, nb - nq);

        while (cy) {
            qh -= lmmp_sub_1_(dstq, dstq, nq, 1);
            cy -= lmmp_add_n_(numa, numa, numb, nb);
        }
    } else {
        mp_limb_t inv21 = lmmp_inv_2_1_(numb[nb - 1], numb[nb - 2]);
        if (nb < DIV_DIVIDE_THRESHOLD)
            qh = lmmp_div_basecase_(dstq, numa, na, numb, nb, inv21);
        else if (nb < DIV_MULINV_L_THRESHOLD || na < 2 * DIV_MULINV_N_THRESHOLD)
            qh = lmmp_div_divide_(dstq, numa, na, numb, nb, inv21);
        else {
            mp_limb_t ni = lmmp_div_inv_size_(nq, nb);
            mp_ptr restrict invappr = TALLOC_TYPE(ni, mp_limb_t);
            lmmp_inv_prediv_(invappr, numb, nb, ni);
            qh = lmmp_div_mulinv_(dstq, numa, na, numb, nb, invappr, ni);
        }
    }
    TEMP_FREE;
    return qh;
}

void lmmp_div_(mp_ptr dstq, mp_ptr dstr, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(numa != NULL && numb != NULL && nb > 0);
    lmmp_param_assert(numb[nb - 1] > 0);
    if (nb == 1) {
        mp_limb_t rem = lmmp_div_1_(dstq, numa, na, *numb);
        if (dstr)
            *dstr = rem;
    } else if (nb == 2) {
        mp_limb_t brem[2];
        brem[0] = numb[0];
        brem[1] = numb[1];
        lmmp_div_2_(dstq, numa, na, brem);
        if (dstr) {
            dstr[0] = brem[0];
            dstr[1] = brem[1];
        }
    } else {
        int adjust = numa[na - 1] >= numb[nb - 1];
        int cnt = lmmp_leading_zeros_(numb[nb - 1]);
        mp_size_t nq = na + adjust - nb;
        if (nq == 0) {
            if (dstr && dstr != numa)
                lmmp_copy(dstr, numa, nb);
            if (dstq)
                dstq[0] = 0;
            return;
        }
        TEMP_DECL;

        if (!dstq)
            dstq = TALLOC_TYPE(na - nb + 1, mp_limb_t);
        dstq[na - nb] = 0;

        if (nq >= nb) {
            mp_ptr restrict numa2 = TALLOC_TYPE(na + 1, mp_limb_t);
            mp_ptr numb2;
            if (cnt) {
                numa2[na] = lmmp_shl_(numa2, numa, na, cnt);
                numb2 = TALLOC_TYPE(nb, mp_limb_t);
                lmmp_shl_(numb2, numb, nb, cnt);
            } else {
                numa2[na] = 0;
                lmmp_copy(numa2, numa, na);
                numb2 = (mp_ptr)numb;
            }

            mp_limb_t inv21 = lmmp_inv_2_1_(numb2[nb - 1], numb2[nb - 2]);
            na += adjust;

            if (nb < DIV_DIVIDE_THRESHOLD)
                lmmp_div_basecase_(dstq, numa2, na, numb2, nb, inv21);
            else if (nb < DIV_MULINV_L_THRESHOLD || na < 2 * DIV_MULINV_N_THRESHOLD)
                lmmp_div_divide_(dstq, numa2, na, numb2, nb, inv21);
            else {
                mp_limb_t ni = lmmp_div_inv_size_(nq, nb);
                mp_ptr invappr = TALLOC_TYPE(ni, mp_limb_t);
                lmmp_inv_prediv_(invappr, numb2, nb, ni);
                lmmp_div_mulinv_(dstq, numa2, na, numb2, nb, invappr, ni);
            }

            if (dstr) {
                if (cnt)
                    lmmp_shr_(dstr, numa2, nb, cnt);
                else
                    lmmp_copy(dstr, numa2, nb);
            }
        } else {
            // nq=na-nb+adj<nb
            //-> na+adj>=2nq+1
            mp_size_t ni = nb - nq;
            mp_ptr numa2, numb2;
            mp_ptr tp = TALLOC_TYPE(nb, mp_limb_t);
            mp_limb_t cy;

            numa2 = TALLOC_TYPE(nq * 2 + 1, mp_limb_t);
            if (cnt) {
                numb2 = TALLOC_TYPE(nq, mp_limb_t);
                lmmp_shl_(numb2, numb + ni, nq, cnt);
                numb2[0] |= numb[ni - 1] >> (LIMB_BITS - cnt);
                cy = lmmp_shl_(numa2, numa + na - 2 * nq, 2 * nq, cnt);
                if (adjust) {
                    numa2[2 * nq] = cy;
                    ++numa2;  // numa2[0] is as significant as numa[ni=na-2nq+adjust]
                } else
                    numa2[0] |= numa[na - 2 * nq - 1] >> (LIMB_BITS - cnt);
            } else {
                numb2 = (mp_ptr)numb + ni;
                lmmp_copy(numa2, numa + na - 2 * nq, 2 * nq);
                if (adjust) {
                    numa2[2 * nq] = 0;
                    ++numa2;
                }
            }

            // now: 0<=numa2<B^2nq, B^nq/2<=numb2<B^nq, and 0<=numa2/numb2<B^nq
            // ignored bits could be seen as fraction part of numa and numb
            // we can prove:  Q<=Qh<=Q+2
            // where Q=floor(numa/numb) is the real quotient
            // Qh=floor(floor(numa)/floor(numb)) as below

            if (nq == 1) {
                lmmp_div_1_s_(dstq, numa2, 2, *numb2);
            } else if (nq == 2) {
                lmmp_div_2_s_(dstq, numa2, 4, numb2);
            } else {
                mp_limb_t inv21 = lmmp_inv_2_1_(numb2[nq - 1], numb2[nq - 2]);

                if (nq < DIV_DIVIDE_THRESHOLD)
                    lmmp_div_basecase_(dstq, numa2, 2 * nq, numb2, nq, inv21);
                else if (nq < DIV_MULINV_N_THRESHOLD)
                    lmmp_div_divide_(dstq, numa2, 2 * nq, numb2, nq, inv21);
                else {
                    mp_size_t inv_ni = lmmp_div_inv_size_(nq, nq);
                    mp_ptr invappr = tp;
                    lmmp_inv_prediv_(invappr, numb2, nq, inv_ni);
                    lmmp_div_mulinv_(dstq, numa2, 2 * nq, numb2, nq, invappr, inv_ni);
                }
            }
            /*
            true remainder = partial remainder - quotient * ignored divisor limbs

            Multiply the first ignored divisor limb by the most significant
            quotient limb.  If that product is > the partial remainder's
            most significant limb, we know the quotient is too large.  This
            test quickly catches most cases where the quotient is too large;
            it catches all cases where the quotient is 2 too large.*/

            mp_limb_t x;
            if (cnt) {
                mp_limb_t dl;
                if (ni < 2)
                    dl = 0;
                else
                    dl = numb[ni - 2];
                x = (numb[ni - 1] << cnt) | (dl >> (LIMB_BITS - cnt));
            } else
                x = numb[ni - 1];
            mp_limb_t h = (x >> LIMB_BITS / 2) * (dstq[nq - 1] >> LIMB_BITS / 2);
            mp_limb_t rnb = 0;  // remainder[nb]
            mp_size_t nr = nq;  // remainder=rnb:[numa2,nr]:[...,ni]

            if (h > numa2[nq - 1]) {
                lmmp_dec(dstq);
                rnb = lmmp_add_n_(numa2, numa2, numb2, nq);
            }

            // if cnt, recover the shift of partial remainder
            // and remove the effect of the partial-ignored numa[ni-1] and numb[ni-1]
            if (cnt) {
                numa2[nq] = rnb;
                ++nr;
                --ni;
                lmmp_shl_(numa2, numa2, nr, LIMB_BITS - cnt);
                numa2[0] |= numa[ni] & (LIMB_MAX >> cnt);
                cy = lmmp_submul_1_(numa2, dstq, nq, numb[ni] & (LIMB_MAX >> cnt));
                rnb = -(numa2[nq] < cy);
                numa2[nq] -= cy;
            }

            if (ni == 0) {
                if (dstr) {
                    if (rnb)
                        lmmp_add_n_(dstr, numa2, numb, nr);
                    else
                        lmmp_copy(dstr, numa2, nr);
                }
            } else {
                tp[nb - 1] = 0;
                if (ni < nq)
                    lmmp_mul_(tp, dstq, nq, numb, ni);
                else
                    lmmp_mul_(tp, numb, ni, dstq, nq);

                if (dstr) {
                    mp_ptr remptr = dstr == numb ? tp : dstr;
                    cy = lmmp_sub_n_(remptr, numa, tp, ni);
                    rnb -= lmmp_sub_nc_(remptr + ni, numa2, tp + ni, nr, cy);
                    if (rnb)
                        lmmp_add_n_(dstr, remptr, numb, nb);
                    else if (dstr != remptr)
                        lmmp_copy(dstr, remptr, nb);
                } else {
                    int hcmp = lmmp_cmp_(numa2, tp + ni, nr);
                    if (hcmp < 0)
                        --rnb;
                    else if (hcmp == 0)
                        rnb -= (lmmp_cmp_(numa, tp, ni) < 0);
                }
            }

            if (rnb)
                lmmp_dec(dstq);
        }

        TEMP_FREE;
    }
}

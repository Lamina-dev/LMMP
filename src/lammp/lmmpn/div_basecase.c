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

#include "../../../include/lammp/impl/mparam.h"
#include "../../../include/lammp/lmmpn.h"


mp_limb_t lmmp_div_basecase_(
    mp_ptr    restrict dstq,
    mp_ptr    restrict numa, 
    mp_size_t            na,
    mp_srcptr restrict numb,
    mp_size_t            nb,
    mp_limb_t inv21
) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(nb >= 3);
    lmmp_param_assert(numb[nb - 1] >= LIMB_B_2);
    mp_size_t nq = na - nb;

    numa += na;

    mp_limb_t qh = lmmp_cmp_(numa - nb, numb, nb) >= 0;
    if (qh) {
        lmmp_sub_n_(numa - nb, numa - nb, numb, nb);
    }

    nb -= 2;
    numa -= 2;

    mp_limb_t d1 = numb[nb + 1], d0 = numb[nb + 0];

    while (nq) {
        mp_limb_t q;
        --numa;
        if (numa[2] == d1 && numa[1] == d0) {
            q = LIMB_MAX;
            lmmp_submul_1_(numa - nb, numb, nb + 2, q);
        } else {
            mp_limb_t cy, cy1;
            q = lmmp_div_3_2_(numa, numb + nb, inv21);
            cy = lmmp_submul_1_(numa - nb, numb, nb, q);
            cy1 = numa[0] < cy;
            numa[0] -= cy;
            cy = numa[1] < cy1;
            numa[1] -= cy1;
            if (cy) {
                lmmp_add_n_(numa - nb, numa - nb, numb, nb + 2);
                --q;
            }
        }
        dstq[--nq] = q;
    }
    return qh;
}

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

#include "../../../include/lammp/numth.h"
#include "../../../include/lammp/lmmpn.h"


/*
 FIXME: 内联lmmp_tailing_zeros_()函数，使用编译器内置函数，可以显著提升性能。
 */

mp_limb_t lmmp_gcd_11_(mp_limb_t u, mp_limb_t v) {
    lmmp_param_assert(u > 0 && v > 0);
    int count, k;
    k = lmmp_tailing_zeros_(u | v);
    u >>= lmmp_tailing_zeros_(u);
    v >>= lmmp_tailing_zeros_(v);
    while (u != v) {
        if (u > v) {
            u -= v;
            count = lmmp_tailing_zeros_(u); 
            u >>= count;
        } else {
            v -= u;
            count = lmmp_tailing_zeros_(v);
            v >>= count;
        }
    }
    return u << k;
}

mp_limb_t lmmp_gcd_1_(mp_srcptr up, mp_size_t un, mp_limb_t vlimb) {
    lmmp_param_assert(un > 0);
    lmmp_param_assert(vlimb > 0);
    mp_limb_t ulimb;
    if (un == 1) {
        ulimb = up[0];
    } else {
        ulimb = lmmp_mod_1_(up, un, vlimb);
    }
    if (ulimb == 0)
        return vlimb;
    else
        return lmmp_gcd_11_(ulimb, vlimb);
}
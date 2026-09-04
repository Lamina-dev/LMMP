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
#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"


mp_limb_t lmmp_gcd_11_(mp_limb_t u, mp_limb_t v) {
    lmmp_param_assert(u > 0 && v > 0);
    int k = lmmp_tailing_zeros_(u | v);   // k = min(tz(u), tz(v))
    u >>= lmmp_tailing_zeros_(u);
    v >>= lmmp_tailing_zeros_(v);
    // u, v 均为奇数；去冗余最低位（隐式最低位表示）
    u >>= 1;
    v >>= 1;
    while (u != v) {
        mp_limb_t t = u - v;
        mp_limb_t m = (mp_limb_t)((slong)t >> 63);   // u<v -> 1
        v += m & t;                                  // v = min(u, v)
        u = (t ^ m) - m;                             // u = |u - v|
        u = (u >> 1) >> lmmp_tailing_zeros_(t);
    }
    return ((u << 1) + 1) << k;
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

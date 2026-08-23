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

#include <math.h>

#include "../../../include/lammp/impl/mparam.h"
#include "../../../include/lammp/numth.h"


ulong lmmp_sqrt_ulong_(ulong a) {
    ulong is;

    is = (ulong)sqrt((double)a);

    is -= (is * is > a);
    if (is == (1ULL << 32))
        is--;
    return is;
}

mp_limb_t lmmp_sqrt_1_(mp_ptr dstr, mp_limb_t x) {
    lmmp_param_assert(x >= LIMB_B_4);
    mp_limb_t s = lmmp_sqrt_ulong_(x);
    *dstr = x - s * s;
    return s;
}

mp_limb_t lmmp_sqrt_2_(mp_ptr dstr, mp_srcptr numa) {
    lmmp_param_assert(numa[1] >= LIMB_B_4);
    mp_limb_t rl, s, q, al, u;
    mp_slimb_t rh;

    s = lmmp_sqrt_1_(&rl, numa[1]);
    al = numa[0];

    //(r:alh)/2
    rl = rl << 31 | al >> 33;
    q = rl / s;
    q -= q >> 32;

    u = rl - s * q;
    s = s << 32 | q;
    rh = u >> 31;
    rl = (u << 33) | (al & (((mp_limb_t)1 << 33) - 1));

    q *= q;
    rh -= rl < q;
    rl -= q;
    if (rh < 0) {
        rl += s;
        rh += rl < s;
        --s;
        rl += s;
        rh += rl < s;
    }

    dstr[0] = rl;
    dstr[1] = rh;
    return s;
}

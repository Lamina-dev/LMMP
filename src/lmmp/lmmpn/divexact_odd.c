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

#include "../../../include/lmmp/impl/divexact.h"
#include "../../../include/lmmp/impl/longlong.h"


mp_limb_t lmmp_divexact_odd_pi_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t d, mp_limb_t di, unsigned shr) {
    mp_limb_t c, l, u, un, lo;
    mp_size_t i;

    lmmp_param_assert(na > 0);
    lmmp_param_assert((d & 1) != 0);
    lmmp_param_assert(shr < LIMB_BITS);
    lmmp_param_assert(d * di == 1);

    if (shr != 0) {
        c = 0;
        u = numa[0];
        for (i = 1; i < na; i++) {
            un = numa[i];
            u = (u >> shr) | (un << (LIMB_BITS - shr));
            l = u - c;
            c = l > u;
            l *= di;
            dst[i - 1] = l;
            u = un;
            _umul64to128_(l, d, &lo, &un);
            c += un;
        }
        u >>= shr;
        l = u - c;
        c = l > u;
        l *= di;
        dst[na - 1] = l;
    } else {
        c = 0;
        i = 0;
        do {
            u = numa[i];
            l = u - c;
            c = l > u;
            l *= di;
            dst[i] = l;
            _umul64to128_(l, d, &lo, &un);
            c += un;
        } while (++i < na);
    }
    return c;
}

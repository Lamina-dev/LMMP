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
 *  This program is distributed in WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/numth.h"


mp_size_t lmmp_gcd_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn) {
    lmmp_param_assert(un > 0 && vn > 0);
    lmmp_param_assert(up != NULL && vp != NULL && dst != NULL);
    lmmp_param_assert(up[un - 1] != 0);
    lmmp_param_assert(vp[vn - 1] != 0);

    if (un < vn) {
        LMMP_SWAP(up, vp, mp_srcptr);
        LMMP_SWAP(un, vn, mp_size_t);
    }
    // u >= v

    if (vn == 1) {
        dst[0] = (un == 1) ? lmmp_gcd_11_(up[0], vp[0]) : lmmp_gcd_1_(up, un, vp[0]);
        return 1;
    }
    if (vn == 2) {
        return (un == 2) ? lmmp_gcd_22_(dst, up, vp) : lmmp_gcd_2_(dst, up, un, vp);
    }
    if (un >= GCD_HGCD_THRESHOLD) {
        return lmmp_gcd_hgcd_(dst, up, un, vp, vn);
    }
    return lmmp_gcd_lehmer_(dst, up, un, vp, vn);
}

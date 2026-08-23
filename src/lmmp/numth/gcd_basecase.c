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

#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

mp_size_t lmmp_gcd_basecase_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp, mp_size_t vn) {
    lmmp_param_assert(un > 0 && vn > 0);
    if (un < vn) {
        LMMP_SWAP(up, vp, mp_srcptr);
        LMMP_SWAP(un, vn, mp_size_t);
    } else if (un == vn) {
        int cmp = lmmp_cmp_(up, vp, un);
        if (cmp < 0) {
            LMMP_SWAP(up, vp, mp_srcptr);
        } else if (cmp == 0) {
            lmmp_copy(dst, up, un);
            return un;
        }
    }
    TEMP_DECL;
#define an un
#define bn vn
    mp_ptr a = TALLOC_TYPE(an, mp_limb_t);
    mp_ptr b = TALLOC_TYPE(bn, mp_limb_t);
    lmmp_copy(a, up, an);
    lmmp_copy(b, vp, bn);
    while (bn > 0 || (bn == 1 && b[0] == 0)) {
        // dst = a % b;
        lmmp_div_(NULL, dst, a, an, b, bn);
        lmmp_copy(a, b, bn);
        an = bn;
        while (dst[bn - 1] == 0 && bn > 0) {
            --bn;
        }
        lmmp_copy(b, dst, bn);
    }
    lmmp_copy(dst, a, an);
    TEMP_FREE;
    return an;
#undef an
#undef bn
}
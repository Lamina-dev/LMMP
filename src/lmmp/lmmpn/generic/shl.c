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

#include "../../../../include/lmmp/lmmpn.h"


mp_limb_t lmmp_shl_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr) {
    if (shr == 0) {
        lmmp_copy(dst, numa, na);
        return 0;
    } else {
        const mp_limb_t rshr = LIMB_BITS - shr;
        mp_limb_t high_limb, low_limb;
        mp_limb_t retval;
        numa += na;
        dst += na;
        low_limb = *--numa;
        retval = low_limb >> rshr;
        high_limb = (low_limb << shr);
        while (--na != 0) {
            low_limb = *--numa;
            *--dst = high_limb | (low_limb >> rshr);
            high_limb = (low_limb << shr);
        }
        *--dst = high_limb;
        return retval;
    }
}

mp_limb_t lmmp_shl_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c) {
    if (shr == 0) {
        lmmp_copy(dst, numa, na);
        return 0;
    } else {
        const mp_limb_t rshr = LIMB_BITS - shr;
        mp_limb_t high_limb, low_limb;
        mp_limb_t retval;
        numa += na;
        dst += na;
        low_limb = *--numa;
        retval = low_limb >> rshr;
        high_limb = (low_limb << shr);
        while (--na != 0) {
            low_limb = *--numa;
            *--dst = high_limb | (low_limb >> rshr);
            high_limb = (low_limb << shr);
        }
        c &= ((mp_limb_t)1 << shr) - 1;
        *--dst = high_limb | c;
        return retval;
    }
}

mp_limb_t lmmp_addshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i, c = 0, mb = 0;

    for (i = 0; i < n; i++) {
        mp_limb_t a, b, r;
        a = numa[i];
        b = (numb[i] << 1) + mb;
        mb = numb[i] >> (LIMB_BITS - 1);
        r = a + c;
        c = (r < c);
        r += b;
        c += (r < b);
        dst[i] = r;
    }
    return c + mb;
}

mp_limb_t lmmp_subshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i, c = 0, mb = 0;

    for (i = 0; i < n; i++) {
        mp_limb_t a, b;
        a = numa[i];
        b = (numb[i] << 1) + mb;
        mb = numb[i] >> (LIMB_BITS - 1);
        b += c;
        c = (b < c);
        c += (a < b);
        dst[i] = a - b;
    }
    return c + mb;
}

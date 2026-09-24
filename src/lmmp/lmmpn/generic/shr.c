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

#include "../../../../include/lmmp/lmmpn.h"


mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr) {
    if (shr == 0) {
        lmmp_copy(dst, numa, na);
        return 0;
    }
    const mp_size_t rshr = LIMB_BITS - shr;
    mp_limb_t retval = numa[0] << rshr;
    mp_size_t i = 0;
    for (; i + 1 < na; i++) dst[i] = (numa[i] >> shr) | (numa[i + 1] << rshr);
    dst[i] = numa[i] >> shr;
    return retval;
}

mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c) {
    if (shr == 0) {
        lmmp_copy(dst, numa, na);
        return 0;
    }
    const mp_size_t rshr = LIMB_BITS - shr;
    mp_limb_t retval = numa[0] << rshr;
    mp_size_t i = 0;
    for (; i + 1 < na; i++) dst[i] = (numa[i] >> shr) | (numa[i + 1] << rshr);
    c &= ~(((mp_limb_t)1 << rshr) - 1);
    dst[i] = (numa[i] >> shr) | c;
    return retval;
}

mp_limb_t lmmp_shr1add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    __uint128_t pr = (__uint128_t)numa[0] + numb[0] + c;
    mp_limb_t l = (mp_limb_t)pr & 1;
    for (mp_size_t i = 1; i < n; i++) {
        __uint128_t cr = (__uint128_t)numa[i] + numb[i] + (mp_limb_t)(pr >> 64);
        dst[i - 1] = ((mp_limb_t)pr >> 1) | ((mp_limb_t)cr << 63);
        pr = cr;
    }
    dst[n - 1] = ((mp_limb_t)pr >> 1) | ((mp_limb_t)(pr >> 64) << 63);
    return l;
}

mp_limb_t lmmp_shr1add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    return lmmp_shr1add_nc_(dst, numa, numb, n, 0);
}

mp_limb_t lmmp_shr1sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    mp_limb_t o = numa[0];
    mp_limb_t u = o - numb[0];
    mp_limb_t pr = u - c;
    mp_limb_t d = (u > o) + (pr > u);
    mp_limb_t l = pr & 1;
    for (mp_size_t i = 1; i < n; i++) {
        o = numa[i];
        u = o - numb[i];
        mp_limb_t r = u - d;
        d = (u > o) + (r > u);
        dst[i - 1] = (pr >> 1) | (r << 63);
        pr = r;
    }
    dst[n - 1] = (pr >> 1) | (d << 63);
    return l;
}

mp_limb_t lmmp_shr1sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    return lmmp_shr1sub_nc_(dst, numa, numb, n, 0);
}

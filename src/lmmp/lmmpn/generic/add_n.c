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


#if defined(__SIZEOF_INT128__)

mp_limb_t lmmp_add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    mp_size_t i = 0;
    mp_limb_t cy = c;

    for (; i + 4 <= n; i += 4) {
        __uint128_t s0 = (__uint128_t)numa[i + 0] + numb[i + 0] + cy;
        __uint128_t s1 = (__uint128_t)numa[i + 1] + numb[i + 1] + (uint64_t)(s0 >> 64);
        __uint128_t s2 = (__uint128_t)numa[i + 2] + numb[i + 2] + (uint64_t)(s1 >> 64);
        __uint128_t s3 = (__uint128_t)numa[i + 3] + numb[i + 3] + (uint64_t)(s2 >> 64);
        dst[i + 0] = (uint64_t)s0;
        dst[i + 1] = (uint64_t)s1;
        dst[i + 2] = (uint64_t)s2;
        dst[i + 3] = (uint64_t)s3;
        cy = (uint64_t)(s3 >> 64);
    }
    for (; i < n; i++) {
        __uint128_t s = (__uint128_t)numa[i] + numb[i] + cy;
        dst[i] = (uint64_t)s;
        cy = (uint64_t)(s >> 64);
    }

    return cy;
}

mp_limb_t lmmp_add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    return lmmp_add_nc_(dst, numa, numb, n, 0);
}

#else /* !__SIZEOF_INT128__ 朴素参考实现 */

mp_limb_t lmmp_add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    mp_size_t i = 0;
    mp_limb_t cy = c;

    for (; i < n; i++) {
        mp_limb_t a, b, r;
        a = numa[i];
        b = numb[i];
        r = a + cy;
        cy = (r < cy);
        r += b;
        cy += (r < b);
        dst[i] = r;
    }

    return cy;
}

mp_limb_t lmmp_add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i = 0;
    mp_limb_t cy = 0;

    for (; i < n; i++) {
        mp_limb_t a, b, r;
        a = numa[i];
        b = numb[i];
        r = a + cy;
        cy = (r < cy);
        r += b;
        cy += (r < b);
        dst[i] = r;
    }

    return cy;
}

#endif /* __SIZEOF_INT128__ */

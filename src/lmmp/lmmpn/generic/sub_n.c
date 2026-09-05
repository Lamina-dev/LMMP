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

mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    /* 单循环：借位提取（shr+neg）位于迭代间串行链上，展开无收益（实测反劣） */
    mp_size_t i = 0;
    mp_limb_t cy = c;

    for (; i < n; i++) {
        __uint128_t d = (__uint128_t)numa[i] - numb[i] - cy;
        dst[i] = (uint64_t)d;
        cy = (uint64_t)(-(uint64_t)(d >> 64));
    }

    return cy;
}

mp_limb_t lmmp_sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    return lmmp_sub_nc_(dst, numa, numb, n, 0);
}

#else /* !__SIZEOF_INT128__ 朴素参考实现 */

mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    mp_size_t i = 0;
    mp_limb_t cy = c;

    for (; i < n; i++) {
        mp_limb_t a, b;
        a = numa[i];
        b = numb[i];
        b += cy;
        cy = (b < cy);
        cy += (a < b);
        dst[i] = a - b;
    }

    return cy;
}

mp_limb_t lmmp_sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i = 0;
    mp_limb_t cy = 0;

    for (; i < n; i++) {
        mp_limb_t a, b;
        a = numa[i];
        b = numb[i];
        b += cy;
        cy = (b < cy);
        cy += (a < b);
        dst[i] = a - b;
    }

    return cy;
}

#endif /* __SIZEOF_INT128__ */

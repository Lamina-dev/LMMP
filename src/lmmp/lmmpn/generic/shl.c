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


mp_limb_t lmmp_shl_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl) {
    if (shl == 0) {
        lmmp_copy(dst, numa, na);
        return 0;
    }
    const mp_size_t rshr = LIMB_BITS - shl;
    mp_limb_t retval = numa[na - 1] >> rshr;
    // dst[0] 先算后存（升序循环可能先写覆盖 numa[0] 的别名位置）
    mp_limb_t lo0 = numa[0] << shl;
    for (mp_size_t i = 1; i < na; i++) dst[i] = (numa[i] << shl) | (numa[i - 1] >> rshr);
    dst[0] = lo0;
    return retval;
}

mp_limb_t lmmp_shl_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shl, mp_limb_t c) {
    if (shl == 0) {
        lmmp_copy(dst, numa, na);
        return 0;
    }
    const mp_size_t rshr = LIMB_BITS - shl;
    mp_limb_t retval = numa[na - 1] >> rshr;
    c &= ((mp_limb_t)1 << shl) - 1;
    // 同 lmmp_shl_：dst[0] 先算后存
    mp_limb_t lo0 = (numa[0] << shl) | c;
    for (mp_size_t i = 1; i < na; i++) dst[i] = (numa[i] << shl) | (numa[i - 1] >> rshr);
    dst[0] = lo0;
    return retval;
}

mp_limb_t lmmp_addshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_limb_t c = 0, mb = 0;
    mp_size_t i = 0;

#define LMMP_ADDSHL1_STEP(k)                                 \
    {                                                        \
        mp_limb_t v = numb[i + (k)];                         \
        mp_limb_t lo = (v << 1) | mb;                        \
        mb = v >> (LIMB_BITS - 1);                           \
        __uint128_t s = (__uint128_t)lo + numa[i + (k)] + c; \
        dst[i + (k)] = (mp_limb_t)s;                         \
        c = (mp_limb_t)(s >> 64);                            \
    }

    for (; i + 4 <= n; i += 4) {
        LMMP_ADDSHL1_STEP(0)
        LMMP_ADDSHL1_STEP(1)
        LMMP_ADDSHL1_STEP(2)
        LMMP_ADDSHL1_STEP(3)
    }
    for (; i < n; i++) {
        LMMP_ADDSHL1_STEP(0)
    }
#undef LMMP_ADDSHL1_STEP
    return c + mb;
}

mp_limb_t lmmp_subshl1_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_limb_t d = 0, mb = 0;
    mp_size_t i = 0;

#define LMMP_SUBSHL1_STEP(k)          \
    {                                 \
        mp_limb_t v = numb[i + (k)];  \
        mp_limb_t lo = (v << 1) | mb; \
        mb = v >> (LIMB_BITS - 1);    \
        mp_limb_t o = numa[i + (k)];  \
        mp_limb_t u = o - lo;         \
        mp_limb_t r = u - d;          \
        d = (u > o) + (r > u);        \
        dst[i + (k)] = r;             \
    }

    for (; i + 4 <= n; i += 4) {
        LMMP_SUBSHL1_STEP(0)
        LMMP_SUBSHL1_STEP(1)
        LMMP_SUBSHL1_STEP(2)
        LMMP_SUBSHL1_STEP(3)
    }
    for (; i < n; i++) {
        LMMP_SUBSHL1_STEP(0)
    }
#undef LMMP_SUBSHL1_STEP
    return d + mb;
}

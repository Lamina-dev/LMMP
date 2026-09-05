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

#include "../../../../include/lmmp/impl/longlong.h"
#include "../../../../include/lmmp/lmmpn.h"


#if defined(__SIZEOF_INT128__)

mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x) {
    mp_limb_t cl = 0;
    mp_size_t i = 0;

    for (; i + 4 <= na; i += 4) {
        __uint128_t a0 = (__uint128_t)numa[i + 0] * x + cl;
        __uint128_t a1 = (__uint128_t)numa[i + 1] * x + (uint64_t)(a0 >> 64);
        __uint128_t a2 = (__uint128_t)numa[i + 2] * x + (uint64_t)(a1 >> 64);
        __uint128_t a3 = (__uint128_t)numa[i + 3] * x + (uint64_t)(a2 >> 64);
        dst[i + 0] = (uint64_t)a0;
        dst[i + 1] = (uint64_t)a1;
        dst[i + 2] = (uint64_t)a2;
        dst[i + 3] = (uint64_t)a3;
        cl = (uint64_t)(a3 >> 64);
    }
    for (; i < na; i++) {
        __uint128_t a = (__uint128_t)numa[i] * x + cl;
        dst[i] = (uint64_t)a;
        cl = (uint64_t)(a >> 64);
    }
    return cl;
}

mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b) {
    mp_limb_t cl = 0;
    mp_size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        __uint128_t a0 = (__uint128_t)numb[i + 0] * b + numa[i + 0] + cl;
        __uint128_t a1 = (__uint128_t)numb[i + 1] * b + numa[i + 1] + (uint64_t)(a0 >> 64);
        __uint128_t a2 = (__uint128_t)numb[i + 2] * b + numa[i + 2] + (uint64_t)(a1 >> 64);
        __uint128_t a3 = (__uint128_t)numb[i + 3] * b + numa[i + 3] + (uint64_t)(a2 >> 64);
        numa[i + 0] = (uint64_t)a0;
        numa[i + 1] = (uint64_t)a1;
        numa[i + 2] = (uint64_t)a2;
        numa[i + 3] = (uint64_t)a3;
        cl = (uint64_t)(a3 >> 64);
    }
    for (; i < n; i++) {
        __uint128_t a = (__uint128_t)numb[i] * b + numa[i] + cl;
        numa[i] = (uint64_t)a;
        cl = (uint64_t)(a >> 64);
    }
    return cl;
}

mp_limb_t lmmp_submul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b) {
    /* 128 位回绕减：a>>64 为 0 或全 1，取反即借位 0/1 */
    mp_limb_t cl = 0;
    mp_size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        __uint128_t a0 = (__uint128_t)numa[i + 0] - (__uint128_t)numb[i + 0] * b - cl;
        __uint128_t a1 = (__uint128_t)numa[i + 1] - (__uint128_t)numb[i + 1] * b - (uint64_t)(-(uint64_t)(a0 >> 64));
        __uint128_t a2 = (__uint128_t)numa[i + 2] - (__uint128_t)numb[i + 2] * b - (uint64_t)(-(uint64_t)(a1 >> 64));
        __uint128_t a3 = (__uint128_t)numa[i + 3] - (__uint128_t)numb[i + 3] * b - (uint64_t)(-(uint64_t)(a2 >> 64));
        numa[i + 0] = (uint64_t)a0;
        numa[i + 1] = (uint64_t)a1;
        numa[i + 2] = (uint64_t)a2;
        numa[i + 3] = (uint64_t)a3;
        cl = (uint64_t)(-(uint64_t)(a3 >> 64));
    }
    for (; i < n; i++) {
        __uint128_t a = (__uint128_t)numa[i] - (__uint128_t)numb[i] * b - cl;
        numa[i] = (uint64_t)a;
        cl = (uint64_t)(-(uint64_t)(a >> 64));
    }
    return cl;
}

#else /* !__SIZEOF_INT128__ 朴素参考实现 */

mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x) {
    mp_limb_t cl = 0;
    mp_size_t i = 0;

    if (dst == numa) {
        for (; i < na; i++) {
            mp_limb_t l, h;
            _umul64to128_(dst[i], x, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            dst[i] = l;
        }
    } else {
        for (; i < na; i++) {
            mp_limb_t l, h;
            _umul64to128_(numa[i], x, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            dst[i] = l;
        }
    }
    return cl;
}

mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b) {
    mp_limb_t cl = 0;
    mp_size_t i = 0;

    if (numa == numb) {
        for (; i < n; i++) {
            mp_limb_t l, h;
            _umul64to128_(numa[i], b, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l = numa[i] + l;
            cl += (l < numa[i]);
            numa[i] = l;
        }
    } else {
        for (; i < n; i++) {
            mp_limb_t l, h;
            _umul64to128_(numb[i], b, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l = numa[i] + l;
            cl += (l < numa[i]);
            numa[i] = l;
        }
    }
    return cl;
}

mp_limb_t lmmp_submul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b) {
    mp_limb_t cl = 0;
    mp_size_t i = 0;

    if (numa == numb) {
        for (; i < n; i++) {
            mp_limb_t l, h;
            _umul64to128_(numa[i], b, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l = numa[i] - l;
            cl += (l > numa[i]);
            numa[i] = l;
        }
    } else {
        for (; i < n; i++) {
            mp_limb_t l, h;
            _umul64to128_(numb[i], b, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l = numa[i] - l;
            cl += (l > numa[i]);
            numa[i] = l;
        }
    }
    return cl;
}

#endif /* __SIZEOF_INT128__ */

void lmmp_mullo_basecase_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_srcptr restrict numb, mp_size_t n) {
    mp_limb_t h;
    h = numa[0] * numb[n - 1];
    if (n != 1) {
        mp_size_t i;
        mp_limb_t b0;

        b0 = *numb++;
        h += numa[n - 1] * b0 + lmmp_mul_1_(dst, numa, n - 1, b0);
        dst++;

        for (i = n - 2; i > 0; i--) {
            b0 = *numb++;
            h += numa[i] * b0 + lmmp_addmul_1_(dst, numa, i, b0);
            dst++;
        }
    }

    dst[0] = h;
}

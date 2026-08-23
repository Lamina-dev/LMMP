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

#include "../../../../include/lammp/lmmpn.h"


mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    mp_size_t i = 0;
    mp_limb_t cy = c;

    for (; i + 4 <= n; i += 4) {
        mp_limb_t a0, b0, r0;
        mp_limb_t a1, b1, r1;
        mp_limb_t a2, b2, r2;
        mp_limb_t a3, b3, r3;

        a0 = numa[i + 0];
        b0 = numb[i + 0];

        a1 = numa[i + 1];
        b1 = numb[i + 1];

        a2 = numa[i + 2];
        b2 = numb[i + 2];

        a3 = numa[i + 3];
        b3 = numb[i + 3];

        b0 += cy;
        cy = (b0 < cy);
        cy += (a0 < b0);
        r0 = a0 - b0;

        b1 += cy;
        cy = (b1 < cy);
        cy += (a1 < b1);
        r1 = a1 - b1;

        b2 += cy;
        cy = (b2 < cy);
        cy += (a2 < b2);
        r2 = a2 - b2;

        b3 += cy;
        cy = (b3 < cy);
        cy += (a3 < b3);
        r3 = a3 - b3;

        dst[i + 0] = r0;
        dst[i + 1] = r1;
        dst[i + 2] = r2;
        dst[i + 3] = r3;
    }

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

    for (; i + 4 <= n; i += 4) {
        mp_limb_t a0, b0, r0;
        mp_limb_t a1, b1, r1;
        mp_limb_t a2, b2, r2;
        mp_limb_t a3, b3, r3;

        a0 = numa[i + 0];
        b0 = numb[i + 0];

        a1 = numa[i + 1];
        b1 = numb[i + 1];

        a2 = numa[i + 2];
        b2 = numb[i + 2];

        a3 = numa[i + 3];
        b3 = numb[i + 3];

        b0 += cy;
        cy = (b0 < cy);
        cy += (a0 < b0);
        r0 = a0 - b0;

        b1 += cy;
        cy = (b1 < cy);
        cy += (a1 < b1);
        r1 = a1 - b1;

        b2 += cy;
        cy = (b2 < cy);
        cy += (a2 < b2);
        r2 = a2 - b2;

        b3 += cy;
        cy = (b3 < cy);
        cy += (a3 < b3);
        r3 = a3 - b3;

        dst[i + 0] = r0;
        dst[i + 1] = r1;
        dst[i + 2] = r2;
        dst[i + 3] = r3;
    }

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

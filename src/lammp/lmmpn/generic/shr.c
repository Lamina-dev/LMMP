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


mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr) {
    if (shr == 0) {
        lmmp_copy(dst, numa, na);
        return 0;
    } else {
        mp_limb_t high_limb, low_limb;
        const mp_size_t rshr = LIMB_BITS - shr;
        mp_limb_t retval;
        high_limb = *numa++;
        retval = (high_limb << rshr);
        low_limb = high_limb >> shr;
        while (--na != 0) {
            high_limb = *numa++;
            *dst++ = low_limb | (high_limb << rshr);
            low_limb = high_limb >> shr;
        }
        *dst = low_limb;
        return retval;
    }
}

mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c) {
    if (shr == 0) {
        lmmp_copy(dst, numa, na);
        return 0;
    } else {
        mp_limb_t high_limb, low_limb;
        const mp_size_t rshr = LIMB_BITS - shr;
        mp_limb_t retval;
        high_limb = *numa++;
        retval = (high_limb << rshr);
        low_limb = high_limb >> shr;
        while (--na != 0) {
            high_limb = *numa++;
            *dst++ = low_limb | (high_limb << rshr);
            low_limb = high_limb >> shr;
        }
        c &= ~(((mp_limb_t)1 << rshr) - 1);
        *dst = low_limb | c;
        return retval;
    }
}

mp_limb_t lmmp_shr1add_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i = 0, c = 0, l = 0;
    mp_limb_t a, b, r;

    a = numa[i];
    b = numb[i];
    r = a + c;
    c = (r < c);
    r += b;
    c += (r < b);
    dst[i] = r >> 1;
    l = r & 1;

    for (i = 1; i < n; i++) {
        a = numa[i];
        b = numb[i];
        r = a + c;
        c = (r < c);
        r += b;
        c += (r < b);
        dst[i - 1] |= r << (LIMB_BITS - 1);
        dst[i] = r >> 1;
    }
    dst[n - 1] |= c << (LIMB_BITS - 1);
    return l;
}

mp_limb_t lmmp_shr1add_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    mp_size_t i = 0, l = 0;
    mp_limb_t a, b, r;

    a = numa[i];
    b = numb[i];
    r = a + c;
    c = (r < c);
    r += b;
    c += (r < b);
    dst[i] = r >> 1;
    l = r & 1;

    for (i = 1; i < n; i++) {
        a = numa[i];
        b = numb[i];
        r = a + c;
        c = (r < c);
        r += b;
        c += (r < b);
        dst[i - 1] |= r << (LIMB_BITS - 1);
        dst[i] = r >> 1;
    }
    dst[n - 1] |= c << (LIMB_BITS - 1);
    return l;
}

mp_limb_t lmmp_shr1sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i = 0, c = 0, l = 0;
    mp_limb_t a, b, r;

    a = numa[i];
    b = numb[i];
    b += c;
    c = (b < c);
    c += (a < b);
    r = a - b;
    dst[i] = r >> 1;
    l = r & 1;

    for (i = 1; i < n; i++) {
        a = numa[i];
        b = numb[i];
        b += c;
        c = (b < c);
        c += (a < b);
        r = a - b;
        dst[i - 1] |= r << (LIMB_BITS - 1);
        dst[i] = r >> 1;
    }
    dst[n - 1] |= c << (LIMB_BITS - 1);
    return l;
}

mp_limb_t lmmp_shr1sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    mp_size_t i = 0, l = 0;
    mp_limb_t a, b, r;

    a = numa[i];
    b = numb[i];
    b += c;
    c = (b < c);
    c += (a < b);
    r = a - b;
    dst[i] = r >> 1;
    l = r & 1;

    for (i = 1; i < n; i++) {
        a = numa[i];
        b = numb[i];
        b += c;
        c = (b < c);
        c += (a < b);
        r = a - b;
        dst[i - 1] |= r << (LIMB_BITS - 1);
        dst[i] = r >> 1;
    }
    dst[n - 1] |= c << (LIMB_BITS - 1);
    return l;
}

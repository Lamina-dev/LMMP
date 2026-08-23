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

#include "../../../../include/lammp/impl/longlong.h"
#include "../../../../include/lammp/lmmpn.h"


void lmmp_sqr_basecase_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na >= 1);

    mp_size_t i;
    mp_limb_t cl, x;

    x = numa[0];
    cl = 0;

    for (i = 0; i + 4 <= na; i += 4) {
        mp_limb_t u0 = numa[i + 0], u1 = numa[i + 1], u2 = numa[i + 2], u3 = numa[i + 3];
        mp_limb_t l0, h0, l1, h1, l2, h2, l3, h3;

        _umul64to128_(u0, x, &l0, &h0);
        l0 += cl;
        cl = (l0 < cl) + h0;
        _umul64to128_(u1, x, &l1, &h1);
        l1 += cl;
        cl = (l1 < cl) + h1;
        _umul64to128_(u2, x, &l2, &h2);
        l2 += cl;
        cl = (l2 < cl) + h2;
        _umul64to128_(u3, x, &l3, &h3);
        l3 += cl;
        cl = (l3 < cl) + h3;

        dst[i + 0] = l0;
        dst[i + 1] = l1;
        dst[i + 2] = l2;
        dst[i + 3] = l3;
    }
    for (; i < na; i++) {
        mp_limb_t u = numa[i], l, h;
        _umul64to128_(u, x, &l, &h);
        l += cl;
        cl = (l < cl) + h;
        dst[i] = l;
    }
    dst[na] = cl;

    dst++;
    mp_size_t nb = na - 1;
    mp_limb_t* nb_ptr = (mp_limb_t*)numa + 1;

    while (nb >= 2) {
        cl = 0;
        x = nb_ptr[0];
        for (i = 0; i + 4 <= na; i += 4) {
            mp_limb_t u0 = numa[i + 0], d0 = dst[i + 0];
            mp_limb_t u1 = numa[i + 1], d1 = dst[i + 1];
            mp_limb_t u2 = numa[i + 2], d2 = dst[i + 2];
            mp_limb_t u3 = numa[i + 3], d3 = dst[i + 3];
            mp_limb_t l0, h0, l1, h1, l2, h2, l3, h3;

            _umul64to128_(u0, x, &l0, &h0);
            l0 += cl;
            cl = (l0 < cl) + h0;
            l0 += d0;
            cl += (l0 < d0);
            dst[i + 0] = l0;
            _umul64to128_(u1, x, &l1, &h1);
            l1 += cl;
            cl = (l1 < cl) + h1;
            l1 += d1;
            cl += (l1 < d1);
            dst[i + 1] = l1;
            _umul64to128_(u2, x, &l2, &h2);
            l2 += cl;
            cl = (l2 < cl) + h2;
            l2 += d2;
            cl += (l2 < d2);
            dst[i + 2] = l2;
            _umul64to128_(u3, x, &l3, &h3);
            l3 += cl;
            cl = (l3 < cl) + h3;
            l3 += d3;
            cl += (l3 < d3);
            dst[i + 3] = l3;
        }
        for (; i < na; i++) {
            mp_limb_t u = numa[i], d = dst[i], l, h;
            _umul64to128_(u, x, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l += d;
            cl += (l < d);
            dst[i] = l;
        }
        dst[na] = cl;
        dst++;

        cl = 0;
        x = nb_ptr[1];
        for (i = 0; i + 4 <= na; i += 4) {
            mp_limb_t u0 = numa[i + 0], d0 = dst[i + 0];
            mp_limb_t u1 = numa[i + 1], d1 = dst[i + 1];
            mp_limb_t u2 = numa[i + 2], d2 = dst[i + 2];
            mp_limb_t u3 = numa[i + 3], d3 = dst[i + 3];
            mp_limb_t l0, h0, l1, h1, l2, h2, l3, h3;

            _umul64to128_(u0, x, &l0, &h0);
            l0 += cl;
            cl = (l0 < cl) + h0;
            l0 += d0;
            cl += (l0 < d0);
            dst[i + 0] = l0;
            _umul64to128_(u1, x, &l1, &h1);
            l1 += cl;
            cl = (l1 < cl) + h1;
            l1 += d1;
            cl += (l1 < d1);
            dst[i + 1] = l1;
            _umul64to128_(u2, x, &l2, &h2);
            l2 += cl;
            cl = (l2 < cl) + h2;
            l2 += d2;
            cl += (l2 < d2);
            dst[i + 2] = l2;
            _umul64to128_(u3, x, &l3, &h3);
            l3 += cl;
            cl = (l3 < cl) + h3;
            l3 += d3;
            cl += (l3 < d3);
            dst[i + 3] = l3;
        }
        for (; i < na; i++) {
            mp_limb_t u = numa[i], d = dst[i], l, h;
            _umul64to128_(u, x, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l += d;
            cl += (l < d);
            dst[i] = l;
        }
        dst[na] = cl;
        dst++;

        nb_ptr += 2;
        nb -= 2;
    }

    while (nb >= 1) {
        cl = 0;
        x = nb_ptr[0];
        for (i = 0; i + 4 <= na; i += 4) {
            mp_limb_t u0 = numa[i + 0], d0 = dst[i + 0];
            mp_limb_t u1 = numa[i + 1], d1 = dst[i + 1];
            mp_limb_t u2 = numa[i + 2], d2 = dst[i + 2];
            mp_limb_t u3 = numa[i + 3], d3 = dst[i + 3];
            mp_limb_t l0, h0, l1, h1, l2, h2, l3, h3;

            _umul64to128_(u0, x, &l0, &h0);
            l0 += cl;
            cl = (l0 < cl) + h0;
            l0 += d0;
            cl += (l0 < d0);
            dst[i + 0] = l0;
            _umul64to128_(u1, x, &l1, &h1);
            l1 += cl;
            cl = (l1 < cl) + h1;
            l1 += d1;
            cl += (l1 < d1);
            dst[i + 1] = l1;
            _umul64to128_(u2, x, &l2, &h2);
            l2 += cl;
            cl = (l2 < cl) + h2;
            l2 += d2;
            cl += (l2 < d2);
            dst[i + 2] = l2;
            _umul64to128_(u3, x, &l3, &h3);
            l3 += cl;
            cl = (l3 < cl) + h3;
            l3 += d3;
            cl += (l3 < d3);
            dst[i + 3] = l3;
        }
        for (; i < na; i++) {
            mp_limb_t u = numa[i], d = dst[i], l, h;
            _umul64to128_(u, x, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l += d;
            cl += (l < d);
            dst[i] = l;
        }
        dst[na] = cl;
        dst++;
        nb_ptr++;
        nb--;
    }
}

void lmmp_mul_basecase_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_size_t            na,
    mp_srcptr restrict numb,
    mp_size_t            nb
) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(nb >= 1);

    mp_size_t i;
    mp_limb_t cl;

    cl = 0;
    mp_limb_t x = numb[0];
    for (i = 0; i + 4 <= na; i += 4) {
        mp_limb_t u0 = numa[i + 0], u1 = numa[i + 1], u2 = numa[i + 2], u3 = numa[i + 3];
        mp_limb_t l0, h0, l1, h1, l2, h2, l3, h3;

        _umul64to128_(u0, x, &l0, &h0);
        l0 += cl;
        cl = (l0 < cl) + h0;
        _umul64to128_(u1, x, &l1, &h1);
        l1 += cl;
        cl = (l1 < cl) + h1;
        _umul64to128_(u2, x, &l2, &h2);
        l2 += cl;
        cl = (l2 < cl) + h2;
        _umul64to128_(u3, x, &l3, &h3);
        l3 += cl;
        cl = (l3 < cl) + h3;

        dst[i + 0] = l0;
        dst[i + 1] = l1;
        dst[i + 2] = l2;
        dst[i + 3] = l3;
    }
    for (; i < na; i++) {
        mp_limb_t u = numa[i], l, h;
        _umul64to128_(u, x, &l, &h);
        l += cl;
        cl = (l < cl) + h;
        dst[i] = l;
    }
    dst[na] = cl;
    dst++;
    numb++;
    nb--;

    while (nb >= 2) {
        // 第一个乘数
        cl = 0;
        x = numb[0];
        for (i = 0; i + 4 <= na; i += 4) {
            mp_limb_t u0 = numa[i + 0], d0 = dst[i + 0];
            mp_limb_t u1 = numa[i + 1], d1 = dst[i + 1];
            mp_limb_t u2 = numa[i + 2], d2 = dst[i + 2];
            mp_limb_t u3 = numa[i + 3], d3 = dst[i + 3];
            mp_limb_t l0, h0, l1, h1, l2, h2, l3, h3;

            _umul64to128_(u0, x, &l0, &h0);
            l0 += cl;
            cl = (l0 < cl) + h0;
            l0 += d0;
            cl += (l0 < d0);
            dst[i + 0] = l0;
            _umul64to128_(u1, x, &l1, &h1);
            l1 += cl;
            cl = (l1 < cl) + h1;
            l1 += d1;
            cl += (l1 < d1);
            dst[i + 1] = l1;
            _umul64to128_(u2, x, &l2, &h2);
            l2 += cl;
            cl = (l2 < cl) + h2;
            l2 += d2;
            cl += (l2 < d2);
            dst[i + 2] = l2;
            _umul64to128_(u3, x, &l3, &h3);
            l3 += cl;
            cl = (l3 < cl) + h3;
            l3 += d3;
            cl += (l3 < d3);
            dst[i + 3] = l3;
        }
        for (; i < na; i++) {
            mp_limb_t u = numa[i], d = dst[i], l, h;
            _umul64to128_(u, x, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l += d;
            cl += (l < d);
            dst[i] = l;
        }
        dst[na] = cl;

        dst++;
        cl = 0;
        x = numb[1];
        for (i = 0; i + 4 <= na; i += 4) {
            mp_limb_t u0 = numa[i + 0], d0 = dst[i + 0];
            mp_limb_t u1 = numa[i + 1], d1 = dst[i + 1];
            mp_limb_t u2 = numa[i + 2], d2 = dst[i + 2];
            mp_limb_t u3 = numa[i + 3], d3 = dst[i + 3];
            mp_limb_t l0, h0, l1, h1, l2, h2, l3, h3;

            _umul64to128_(u0, x, &l0, &h0);
            l0 += cl;
            cl = (l0 < cl) + h0;
            l0 += d0;
            cl += (l0 < d0);
            dst[i + 0] = l0;
            _umul64to128_(u1, x, &l1, &h1);
            l1 += cl;
            cl = (l1 < cl) + h1;
            l1 += d1;
            cl += (l1 < d1);
            dst[i + 1] = l1;
            _umul64to128_(u2, x, &l2, &h2);
            l2 += cl;
            cl = (l2 < cl) + h2;
            l2 += d2;
            cl += (l2 < d2);
            dst[i + 2] = l2;
            _umul64to128_(u3, x, &l3, &h3);
            l3 += cl;
            cl = (l3 < cl) + h3;
            l3 += d3;
            cl += (l3 < d3);
            dst[i + 3] = l3;
        }
        for (; i < na; i++) {
            mp_limb_t u = numa[i], d = dst[i], l, h;
            _umul64to128_(u, x, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l += d;
            cl += (l < d);
            dst[i] = l;
        }
        dst[na] = cl;

        dst++;
        numb += 2;
        nb -= 2;
    }

    while (nb >= 1) {
        cl = 0;
        x = numb[0];
        for (i = 0; i + 4 <= na; i += 4) {
            mp_limb_t u0 = numa[i + 0], d0 = dst[i + 0];
            mp_limb_t u1 = numa[i + 1], d1 = dst[i + 1];
            mp_limb_t u2 = numa[i + 2], d2 = dst[i + 2];
            mp_limb_t u3 = numa[i + 3], d3 = dst[i + 3];
            mp_limb_t l0, h0, l1, h1, l2, h2, l3, h3;

            _umul64to128_(u0, x, &l0, &h0);
            l0 += cl;
            cl = (l0 < cl) + h0;
            l0 += d0;
            cl += (l0 < d0);
            dst[i + 0] = l0;
            _umul64to128_(u1, x, &l1, &h1);
            l1 += cl;
            cl = (l1 < cl) + h1;
            l1 += d1;
            cl += (l1 < d1);
            dst[i + 1] = l1;
            _umul64to128_(u2, x, &l2, &h2);
            l2 += cl;
            cl = (l2 < cl) + h2;
            l2 += d2;
            cl += (l2 < d2);
            dst[i + 2] = l2;
            _umul64to128_(u3, x, &l3, &h3);
            l3 += cl;
            cl = (l3 < cl) + h3;
            l3 += d3;
            cl += (l3 < d3);
            dst[i + 3] = l3;
        }
        for (; i < na; i++) {
            mp_limb_t u = numa[i], d = dst[i], l, h;
            _umul64to128_(u, x, &l, &h);
            l += cl;
            cl = (l < cl) + h;
            l += d;
            cl += (l < d);
            dst[i] = l;
        }
        dst[na] = cl;
        dst++;
        numb++;
        nb--;
    }
}
/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "../../../include/lmmp/numth.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/trialdiv.h"

LMMP_VECT_CONSERVATIVE


#define MONT63_MAX ((ulong)(0x7fffffffffffffff))

/**
 * @brief 蒙域约化 REDC
 * @warning m<2^63
 */
static inline ulong mont63_reduce(u128 t, ulong m, ulong m_inv) {
    // m < 2^63 时 k*m + t < 2^127，无 129 位溢出
    ulong k = _u128low(t) * m_inv;
    u128 sum = (u128)k * m + t;
    ulong hi = _u128high(sum);
    return hi >= m ? hi - m : hi;
}

/**
 * @brief 蒙域乘：a*b*R^-1 mod m（R=B）
 */
static inline ulong mont63_mul(ulong a, ulong b, ulong m, ulong m_inv) {
    return mont63_reduce((u128)a * b, m, m_inv);
}

mp_limb_t lmmp_mod_4chunk_1_(mp_srcptr numa, mp_size_t na, mp_limb_t d, mp_limb_t dinv, mp_limb_t r2,
                             mp_limb_t base, mp_limb_t mi) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa != NULL);
    lmmp_param_assert(d % 2 == 1);
    lmmp_param_assert(d <= MONT63_MAX);
    lmmp_param_assert(dinv * d == 1);
    lmmp_param_assert(dinv + mi == 0);
    lmmp_param_assert(base < d);
    lmmp_param_assert(r2 < d);

    mp_size_t L = na >> 2, t = na & 3;
    ulong bw = r2, wm = base;
    for (mp_size_t ee = L; ee; ee >>= 1) {
        if (ee & 1) wm = mont63_mul(wm, bw, d, mi);
        bw = mont63_mul(bw, bw, d, mi);
    }
    ulong bt = r2; /* mont(B^t)：t=0 时终段有 if(t) 守卫 */
    for (mp_size_t k = 1; k < t; ++k) bt = mont63_mul(bt, r2, d, mi);

    const mp_limb_t *p0 = numa + t,         *p1 = numa + t + L, *p2 = numa + t + 2 * L,
                    *p3 = numa + t + 3 * L,  *e = numa + t + L;
    ulong c0 = 0, c1 = 0, c2 = 0, c3 = 0;
    while (p0 < e) {
        ulong s, l, q;
        s = *p0++;
        l = s - c0;
        q = l * dinv;
        c0 = _u128high((u128)q * d) + (s < c0);
        s = *p1++;
        l = s - c1;
        q = l * dinv;
        c1 = _u128high((u128)q * d) + (s < c1);
        s = *p2++;
        l = s - c2;
        q = l * dinv;
        c2 = _u128high((u128)q * d) + (s < c2);
        s = *p3++;
        l = s - c3;
        q = l * dinv;
        c3 = _u128high((u128)q * d) + (s < c3);
    }
    ulong cb = 0;
    for (mp_size_t i = 0; i < t; ++i) {
        ulong s = numa[i], l = s - cb;
        ulong q = l * dinv;
        cb = _u128high((u128)q * d) + (s < cb);
    }
    ulong cj[4] = {c0, c1, c2, c3}, wT = wm, acc = 0;
    for (int j = 0; j < 3; ++j) {
        acc += mont63_mul(cj[j], wm, d, mi);
        acc = acc >= d ? acc - d : acc;
        wm = mont63_mul(wm, wT, d, mi);
    }
    acc += mont63_mul(cj[3], wm, d, mi);
    acc = acc >= d ? acc - d : acc;
    acc += cb; /* 底组项权重 B^t*w^0 */
    acc = acc >= d ? acc - d : acc;
    if (t) acc = mont63_mul(acc, bt, d, mi); /* ×B^t：N ≡ -B^t*acc */
    return acc ? d - acc : 0;
}

mp_limb_t lmmp_mod_8chunk_1_(mp_srcptr numa, mp_size_t na, mp_limb_t d, mp_limb_t dinv, mp_limb_t r2,
                             mp_limb_t base, mp_limb_t mi) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa != NULL);
    lmmp_param_assert(d % 2 == 1);
    lmmp_param_assert(d <= MONT63_MAX);
    lmmp_param_assert(dinv * d == 1);
    lmmp_param_assert(dinv + mi == 0);
    lmmp_param_assert(base < d);
    lmmp_param_assert(r2 < d);

    mp_size_t L = na >> 3, t = na & 7;
    ulong bw = r2, wm = base;
    for (mp_size_t ee = L; ee; ee >>= 1) {
        if (ee & 1) wm = mont63_mul(wm, bw, d, mi);
        bw = mont63_mul(bw, bw, d, mi);
    }
    ulong bt = r2; /* mont(B^t)：t=0 时终段有 if(t) 守卫 */
    for (mp_size_t k = 1; k < t; ++k) bt = mont63_mul(bt, r2, d, mi);

    const mp_limb_t *p0 = numa + t,         *p1 = numa + t + L,     *p2 = numa + t + 2 * L,
                    *p3 = numa + t + 3 * L, *p4 = numa + t + 4 * L, *p5 = numa + t + 5 * L,
                    *p6 = numa + t + 6 * L, *p7 = numa + t + 7 * L,  *e = numa + t + L;
    ulong c0 = 0, c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = 0, c6 = 0, c7 = 0;
    while (p0 < e) {
        ulong s, l, q;
        s = *p0++;
        l = s - c0;
        q = l * dinv;
        c0 = _u128high((u128)q * d) + (s < c0);
        s = *p1++;
        l = s - c1;
        q = l * dinv;
        c1 = _u128high((u128)q * d) + (s < c1);
        s = *p2++;
        l = s - c2;
        q = l * dinv;
        c2 = _u128high((u128)q * d) + (s < c2);
        s = *p3++;
        l = s - c3;
        q = l * dinv;
        c3 = _u128high((u128)q * d) + (s < c3);
        s = *p4++;
        l = s - c4;
        q = l * dinv;
        c4 = _u128high((u128)q * d) + (s < c4);
        s = *p5++;
        l = s - c5;
        q = l * dinv;
        c5 = _u128high((u128)q * d) + (s < c5);
        s = *p6++;
        l = s - c6;
        q = l * dinv;
        c6 = _u128high((u128)q * d) + (s < c6);
        s = *p7++;
        l = s - c7;
        q = l * dinv;
        c7 = _u128high((u128)q * d) + (s < c7);
    }
    ulong cb = 0;
    for (mp_size_t i = 0; i < t; ++i) {
        ulong s = numa[i], l = s - cb;
        ulong q = l * dinv;
        cb = _u128high((u128)q * d) + (s < cb);
    }
    ulong cj[8] = {c0, c1, c2, c3, c4, c5, c6, c7}, wT = wm, acc = 0;
    for (int j = 0; j < 7; ++j) {
        acc += mont63_mul(cj[j], wm, d, mi);
        acc = acc >= d ? acc - d : acc;
        wm = mont63_mul(wm, wT, d, mi);
    }
    acc += mont63_mul(cj[7], wm, d, mi);
    acc = acc >= d ? acc - d : acc;
    acc += cb; /* 底组项权重 B^t*w^0 */
    acc = acc >= d ? acc - d : acc;
    if (t) acc = mont63_mul(acc, bt, d, mi); /* ×B^t：N ≡ -B^t*acc */
    return acc ? d - acc : 0;
}

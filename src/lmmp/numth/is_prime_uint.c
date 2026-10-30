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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/is_prime.h"
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/prime_table.h"
#include "../../../include/lmmp/numth.h"


static inline ulong urem32_preinv(ulong t, ulong n, ulong dinv) {
    ulong q = _umul64to64hi_(t, dinv);
    ulong r = t - q * n;
    if (r >= n) r += n;
    return r;
}

static void powmod2b_u32(uint b, uint d, int ebits, uint n, ulong dinv, uint* py2, uint* pyb) {
    ulong xb = b;
    if (xb >= n)
        xb = urem32_preinv(xb, n, dinv);

    if (ebits <= 6) {
        ulong x2 = urem32_preinv((ulong)1 << d, n, dinv);
        for (int k = ebits - 2; k >= 0; k--) {
            xb = urem32_preinv(xb * xb, n, dinv);
            if ((d >> k) & 1)
                xb = urem32_preinv(xb * b, n, dinv);
        }
        *py2 = (uint)x2;
        *pyb = (uint)xb;
        return;
    }

    ulong x2 = urem32_preinv((ulong)1 << (d >> (ebits - 6)), n, dinv);
    for (int k = ebits - 2; k >= ebits - 6; k--) {
        xb = urem32_preinv(xb * xb, n, dinv);
        if ((d >> k) & 1)
            xb = urem32_preinv(xb * b, n, dinv);
    }
    if (n < (1u << 31)) {
        for (int k = ebits - 7; k >= 0; k--) {
            x2 = urem32_preinv(x2 * x2, n, dinv);
            x2 <<= (d >> k) & 1;
            xb = urem32_preinv(xb * xb, n, dinv);
            if ((d >> k) & 1)
                xb = urem32_preinv(xb * b, n, dinv);
        }
        if (x2 >= n) x2 -= n;
    } else {
        for (int k = ebits - 7; k >= 0; k--) {
            x2 = urem32_preinv(x2 * x2, n, dinv);
            x2 <<= (d >> k) & 1;
            if (x2 >= n) x2 -= n;
            xb = urem32_preinv(xb * xb, n, dinv);
            if ((d >> k) & 1)
                xb = urem32_preinv(xb * b, n, dinv);
        }
    }
    *py2 = (uint)x2;
    *pyb = (uint)xb;
}

static int sprp_check_u32(uint y, ulong t, uint n, ulong dinv) {
    if (y == 1 || y == n - 1)
        return 1;
    for (t--; t > 0; t--) {
        ulong v = y;
        v = urem32_preinv(v * v, n, dinv);
        y = (uint)v;
        if (y == n - 1) return 1;
        if (y == 1) return 0;
    }
    return 0;
}

bool lmmp_is_prime_uint_notrial_(uint n) {
    lmmp_param_assert(n > 2);
    lmmp_param_assert(n % 2 == 1);

    uint base = dj_base49[((0x3AC69A35UL * n) & 0xFFFFFFFFUL) >> 21] + 3;
    if (n % base == 0 || base % n == 0) {
        if (n > base)
            return false;
        for (uint p = 3; p * p <= n; p += 2)
            if (n % p == 0) return false;
        return true;
    }

    ulong dinv = MP_ULONG_MAX / n + 1;
    ulong d;
    ulong t;
    ctz_shr_u64(d, n - 1, t);
    int ebits = lmmp_limb_bits_(d);

    uint y2, yb;
    powmod2b_u32(base, (uint)d, ebits, n, dinv, &y2, &yb);
    if (!sprp_check_u32(y2, t, n, dinv))
        return false;
    return sprp_check_u32(yb, t, n, dinv);
}

bool lmmp_is_prime_uint_(uint n) {
    if (n == 2)
        return true;
    if (n % 2 == 0 || n <= 1)
        return false;
    int judge = lmmp_is_prime_table_(n);
    if (judge == 0) {
        return false;
    } else if (judge == 1) {
        return true;
    }

    if (trial_div35711(n))
        return false;

    return lmmp_is_prime_uint_notrial_(n);
}

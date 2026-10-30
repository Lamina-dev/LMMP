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
#include "../../../include/lmmp/impl/longlong.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"

/*
mont63 只能用于小于 2^63 的数，否则会溢出导致计算结果不正确
mont64 可以用于任意大小的数，但由于考虑了溢出的情况，所以速度理论上会慢一些（并未详细测试）

（本文件与 is_prime_ulong.c 各持一份同源拷贝，库内惯例勿强行收敛）
*/
#define MONT63_MAX ((ulong)(0x7fffffffffffffff))

static inline ulong mont64_reduce(u128 t, ulong m, ulong m_inv) {
    // sum = k*m + t < 2^129，第 129 位 c 由 128 位回绕捕获
    ulong k = _u128low(t) * m_inv;
    u128 km = (u128)k * m;
    u128 sum = km + t;
    uint c = sum < km;
    ulong hi = _u128high(sum);
    if (c) return hi - m;  // 2^128 + hi - m 的低 128 位高位即结果
    return hi >= m ? hi - m : hi;
}

static inline ulong mont_R2(ulong m) {
    mp_bitcnt_t shift = 0;
    clz_shl_u64(m, m, shift);
    mp_limb_t r[3] = {0, 0, 1ull << shift};
    mp_limb_t q[2];
    lmmp_div_1_s_(q, r, 3, m);
    return r[0] >> shift;
}

static inline ulong mont64_R2(ulong m) {
    return mont_R2(m);
}

static inline ulong to_mont64(ulong x, ulong R2, ulong m, ulong m_inv) {
    return mont64_reduce((u128)x * R2, m, m_inv);
}

static inline ulong from_mont64(ulong x, ulong m, ulong m_inv) {
    return mont64_reduce(x, m, m_inv);
}

static inline ulong mont64_mul(ulong a, ulong b, ulong m, ulong m_inv) {
    return mont64_reduce((u128)a * b, m, m_inv);
}

static inline ulong mont63_reduce(u128 t, ulong m, ulong m_inv) {
    // m < 2^63 时 k*m + t < 2^127，无 129 位溢出
    ulong k = _u128low(t) * m_inv;
    u128 sum = (u128)k * m + t;
    ulong hi = _u128high(sum);
    return hi >= m ? hi - m : hi;
}

static inline ulong mont63_R2(ulong m) {
    return mont_R2(m);
}

static inline ulong to_mont63(ulong x, ulong R2, ulong m, ulong m_inv) {
    return mont63_reduce((u128)x * R2, m, m_inv);
}

static inline ulong from_mont63(ulong x, ulong m, ulong m_inv) {
    return mont63_reduce(x, m, m_inv);
}

static inline ulong mont63_mul(ulong a, ulong b, ulong m, ulong m_inv) {
    return mont63_reduce((u128)a * b, m, m_inv);
}

uint lmmp_powmod_uint_odd_(uint base, ulong exp, uint mod) {
    lmmp_param_assert(mod > base);
    lmmp_param_assert(mod % 2 == 1);
    lmmp_param_assert(mod > 1);
    ulong dst = 1;
    ulong b = base;
    _udiv64_t binv = _udiv64_gen(mod);
    ulong q;
    while (1) {
        if (exp & 1) {
            dst *= b;
            q = _udiv64by64_q_preinv(dst, &binv);
            dst -= q * mod;
        }
        exp >>= 1;
        if (exp == 0)
            break;
        b *= b;
        q = _udiv64by64_q_preinv(b, &binv);
        b -= q * mod;
    }
    return dst;
}

ulong lmmp_powmod_ulong_odd_(ulong base, ulong exp, ulong mod) {
    lmmp_param_assert(mod > base);
    lmmp_param_assert(mod % 2 == 1);
    lmmp_param_assert(mod > 1);
    if (mod <= MP_UINT_MAX)
        return lmmp_powmod_uint_odd_(base, exp, mod);
    else if (mod <= MONT63_MAX) {
        ulong R2 = mont63_R2(mod);
        ulong m_inv = lmmp_binvert_ulong_(mod);
        m_inv = -m_inv;
        ulong dst = to_mont63(1, R2, mod, m_inv);
        base = to_mont63(base, R2, mod, m_inv);
        while (1) {
            if (exp & 1)
                dst = mont63_mul(dst, base, mod, m_inv);
            exp >>= 1;
            if (exp == 0)
                break;
            base = mont63_mul(base, base, mod, m_inv);
        }
        return from_mont63(dst, mod, m_inv);
    } else {
        ulong R2 = mont64_R2(mod);
        ulong m_inv = lmmp_binvert_ulong_(mod);
        m_inv = -m_inv;
        ulong dst = to_mont64(1, R2, mod, m_inv);
        base = to_mont64(base, R2, mod, m_inv);
        while (1) {
            if (exp & 1)
                dst = mont64_mul(dst, base, mod, m_inv);
            exp >>= 1;
            if (exp == 0)
                break;
            base = mont64_mul(base, base, mod, m_inv);
        }
        return from_mont64(dst, mod, m_inv);
    }
}

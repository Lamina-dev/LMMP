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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


/**
 * @brief 128 位尾零数
 * @return if (lo != 0) return ctz(lo); else return 64 + ctz(hi);
 */
static inline int ctz_u128(mp_limb_t lo, mp_limb_t hi) {
    return lo ? lmmp_tailing_zeros_(lo) : (64 + lmmp_tailing_zeros_(hi));
}

static inline void lshr_u128(mp_limb_t* lo, mp_limb_t* hi, int k) {
    if (k == 0)
        return;
    if (k < 64) {
        *lo = (*lo >> k) | (*hi << (64 - k));
        *hi >>= k;
    } else {
        *lo = *hi >> (k - 64);
        *hi = 0;
    }
}

static inline void lshl_u128(mp_limb_t* lo, mp_limb_t* hi, int k) {
    if (k == 0)
        return;
    if (k < 64) {
        *hi = (*lo >> (64 - k)) | (*hi << k);
        *lo <<= k;
    } else {
        *hi = *lo << (k - 64);
        *lo = 0;
    }
}

mp_size_t lmmp_gcd_22_(mp_ptr dst, mp_srcptr up, mp_srcptr vp) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(up != NULL);
    lmmp_param_assert(vp != NULL);
    lmmp_param_assert(!(up[1] == 0 && up[0] == 0));
    lmmp_param_assert(!(vp[1] == 0 && vp[0] == 0));
    mp_limb_t u0 = up[0], u1 = up[1];
    mp_limb_t v0 = vp[0], v1 = vp[1];

    if ((u1 | v1) == 0) {
        dst[0] = lmmp_gcd_11_(u0, v0);
        dst[1] = 0;
        return 1;
    }

    // cnt = min(tz(u), tz(v)) 为 gcd 中的 2 因子幂
    int ku = ctz_u128(u0, u1);
    int kv = ctz_u128(v0, v1);
    int cnt = ku < kv ? ku : kv;
    lshr_u128(&u0, &u1, ku);
    lshr_u128(&v0, &v1, kv);
    // u, v 均为奇数；去冗余最低位（隐式最低位表示），腾出符号位
    u0 = (u0 >> 1) | (u1 << 63);
    u1 >>= 1;
    v0 = (v0 >> 1) | (v1 << 63);
    v1 >>= 1;

    while (u1 | v1) {   // 至少一方仍为双 limb
        mp_limb_t t0 = u0 - v0;
        mp_limb_t t1 = u1 - v1 - (u0 < v0);
        mp_limb_t m = (mp_limb_t)(t1 >> 63);   // u<v -> 1
        if (t0 == 0) {
            if (t1 == 0)
                break;

            v1 += m & t1;                // v1 = min(u1, v1)（v0 == u0，无需更新）
            u0 = (t1 ^ m) - m;           // |u1 - v1|
            u0 >>= lmmp_tailing_zeros_(t1) + 1;
            u1 = 0;
        } else {
            int c = lmmp_tailing_zeros_(t0) + 1;
            // v = min(u, v)（128 位加法 v += m & t）
            mp_limb_t w0 = m & t0;
            mp_limb_t w1 = m & t1;
            v0 += w0;
            v1 += w1 + (v0 < w0);

            u0 = (t0 ^ m) - m;
            u1 = t1 ^ m;
            if (c == 64) {
                u0 = u1;
                u1 = 0;
            } else {
                u0 = (u0 >> c) | (u1 << (64 - c));
                u1 >>= c;
            }
        }
    }

    mp_limb_t g0, g1;   // 奇数 gcd
    if (u1 | v1) {
        g0 = (u0 << 1) | 1;
        g1 = (u1 << 1) | (u0 >> 63);
    } else {
        while ((u0 | v0) >> 63) {   // 任一 >= 2^63 则真值 >= 2^64，至多两次迭代
            mp_limb_t t = u0 - v0;
            if (t == 0)
                break;   // u == v
            mp_limb_t m = -(mp_limb_t)(u0 < v0);   // 符号掩码
            v0 += m & t;                           // v = min(u0, v0)
            u0 = (t ^ m) - m;                      // |u0 - v0|
            u0 = (u0 >> 1) >> lmmp_tailing_zeros_(t);
        }
        if (u0 == v0) {
            g0 = (u0 << 1) | 1;
            g1 = u0 >> 63;
        } else {
            g0 = lmmp_gcd_11_((u0 << 1) + 1, (v0 << 1) + 1);
            g1 = 0;
        }
    }

    dst[0] = g0;
    dst[1] = g1;
    if (cnt > 0)
        lshl_u128(&dst[0], &dst[1], cnt);
    return dst[1] == 0 ? 1 : 2;
}

mp_size_t lmmp_gcd_2_(mp_ptr dst, mp_srcptr up, mp_size_t un, mp_srcptr vp) {
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(up != NULL);
    lmmp_param_assert(vp != NULL);
    lmmp_param_assert(un > 2);
    lmmp_param_assert(vp[1] != 0);
    mp_limb_t u[2] = {vp[0], vp[1]};
    lmmp_mod_2_(up, un, u);
    if (u[1] == 0 && u[0] == 0) {
        dst[0] = vp[0];
        dst[1] = vp[1];
        return 2;
    } else {
        return lmmp_gcd_22_(dst, vp, u);
    }
}

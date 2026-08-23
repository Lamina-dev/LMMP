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

#ifndef __LMMP_DIVEXACT_H__
#define __LMMP_DIVEXACT_H__

#include "../lmmpn.h"

#define MODLIMB_INVERSE_3 ((mp_limb_t)0xAAAAAAAAAAAAAAAB)

/**
 * @brief 精确除以3（[dst,na] = [numa,na] / 3）
 * @param dst 结果存储位置
 * @param numa 被除数
 * @param na 被除数长度
 * @warning eqsep(dst,numa), na>0
 */
static inline void lmmp_divexact_by3_(mp_ptr dst, mp_srcptr numa, mp_size_t na) {
    mp_limb_t c = 0;
    mp_limb_t l, q, s;
    mp_size_t i = 0;
    do {
        s = numa[i];
        l = s - c;
        c = l > s;
        q = l * MODLIMB_INVERSE_3;
        dst[i] = q;
        l = q + q;
        c += l < q;
        l += q;
        c += l < q;
    } while (++i < na);
}

#define MODLIMB_INVERSE_9 ((mp_limb_t)0x8E38E38E38E38E39)

/**
 * @brief 精确除以9（[dst,na] = [numa,na] / 9）
 * @param dst 结果存储位置
 * @param numa 被除数
 * @param na 被除数长度
 * @warning eqsep(dst,numa), na>0
 */
static inline void lmmp_divexact_by9_(mp_ptr dst, mp_srcptr numa, mp_size_t na) {
    mp_limb_t c = 0;
    mp_limb_t l, q, s, t, carry;
    mp_size_t i = 0;
    do {
        s = numa[i];
        l = s - c;
        c = l > s;
        q = l * MODLIMB_INVERSE_9;
        dst[i] = q;
        // carry from 9*q = (q<<3) + q
        t = q << 3;
        carry = (q >> (LIMB_BITS - 3)) + ((t + q) < t);
        c += carry;
    } while (++i < na);
}

#define MODLIMB_INVERSE_15 ((mp_limb_t)0xEEEEEEEEEEEEEEEF)

/**
 * @brief 精确除以15（[dst,na] = [numa,na] / 15）
 * @param dst 结果存储位置
 * @param numa 被除数
 * @param na 被除数长度
 * @warning eqsep(dst,numa), na>0
 */
static inline void lmmp_divexact_by15_(mp_ptr dst, mp_srcptr numa, mp_size_t na) {
    mp_limb_t c = 0;
    mp_limb_t l, q, s, t, carry;
    mp_size_t i = 0;
    do {
        s = numa[i];
        l = s - c;
        c = l > s;
        q = l * MODLIMB_INVERSE_15;
        dst[i] = q;
        // carry from 15*q = (q<<4) - q
        t = q << 4;
        carry = (q >> (LIMB_BITS - 4)) - (t < q);
        c += carry;
    } while (++i < na);
}

#endif // __LMMP_DIVEXACT_H__
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

#include "../../../include/lammp/impl/inlines.h"
#include "../../../include/lammp/impl/mparam.h"
#include "../../../include/lammp/impl/tmp_alloc.h"
#include "../../../include/lammp/lmmpn.h"
#include "../../../include/lammp/numth.h"


#define mul_b(_i_)                                 \
    lmmp_mul_(dst, sq, rn, b##_i_, b##_i_##n);     \
    rn += b##_i_##n;                               \
    rn -= (dst[rn - 1] == 0) ? 1 : 0

mp_size_t lmmp_pow_basecase_(mp_ptr restrict dst, mp_size_t rn, mp_srcptr restrict base, mp_size_t n, ulong exp) {
    lmmp_param_assert(exp >= 3);
    lmmp_param_assert(exp % 2 == 1);
    TEMP_DECL;

#define b1 base
#define b1n n
    mp_ptr restrict sq = TALLOC_TYPE(rn, mp_limb_t);
    rn = n;
    lmmp_copy(dst, base, n);
    lmmp_sqr_(sq, dst, rn);
    rn <<= 1;
    rn -= (sq[rn - 1] == 0) ? 1 : 0;
    int lz = lmmp_leading_zeros_(exp);
    int i = 62 - lz;
    exp <<= lz + 1;
/*
    For the intermediate 0, we will skip it entirely until the next 1, 
    and then perform a multiplication. This can reduce the extra copying 
    caused by sparse 1s and improve efficiency.
 */
    for ( ; i > 0; ) {
        lz = lmmp_leading_zeros_(exp);
        if (lz == 0) {
            mul_b(1);

            lmmp_sqr_(sq, dst, rn);
            rn <<= 1;
            rn -= (sq[rn - 1] == 0) ? 1 : 0;
            --i;
            exp <<= 1;
        } else {
            int j = 2;
            if (lz & 1) {
                lmmp_copy(dst, sq, rn);
                lmmp_sqr_(sq, dst, rn);
                rn <<= 1;
                rn -= (sq[rn - 1] == 0);
                ++j;
                for (; j <= lz; j += 2) {
                    lmmp_sqr_(dst, sq, rn);
                    rn <<= 1;
                    rn -= (dst[rn - 1] == 0);
                    lmmp_sqr_(sq, dst, rn);
                    rn <<= 1;
                    rn -= (sq[rn - 1] == 0);
                }
            } else {
                for (; j <= lz; j += 2) {
                    lmmp_sqr_(dst, sq, rn);
                    rn <<= 1;
                    rn -= (dst[rn - 1] == 0);
                    lmmp_sqr_(sq, dst, rn);
                    rn <<= 1;
                    rn -= (sq[rn - 1] == 0);
                }
            }

            i -= lz;
            exp <<= lz;
        }
    }
    lmmp_debug_assert(exp == LIMB_B_2);

    mul_b(1);
    
    TEMP_FREE;
    return rn;

#undef b1
#undef b1n
#undef new_b
}
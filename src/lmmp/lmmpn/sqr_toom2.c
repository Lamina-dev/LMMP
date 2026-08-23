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

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/lmmpn.h"


#if MUL_TOOM22_THRESHOLD < MUL_TOOM33_THRESHOLD
#define lmmp_sqr_(dst, numa, n)                 \
    if ((n) < MUL_TOOM22_THRESHOLD)             \
        lmmp_sqr_basecase_((dst), (numa), (n)); \
    else                                        \
        lmmp_sqr_toom2_((dst), (numa), (n))
#endif

/*
Evaluate in: -1, 0, +inf

   <-s--><--n-->
   |-a1-|--a0--|

v0  =  a0    ^2  #   A(0)^2
vm1 = (a0-a1)^2  #  A(-1)^2
vinf=     a1 ^2  # A(inf)^2
*/

void lmmp_sqr_toom2_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(numa!= NULL);
    TEMP_S_DECL;
    mp_size_t s = na >> 1, n = na - s;
    mp_limb_t* vm1 = SALLOC_TYPE(2 * n, mp_limb_t);
    mp_slimb_t cy, cy2;

#define a0 numa
#define a1 (numa + n)
#define asm1 dst

    if (s == n) {
        if (lmmp_cmp_(a0, a1, n) < 0)
            lmmp_sub_n_(asm1, a1, a0, n);
        else
            lmmp_sub_n_(asm1, a0, a1, n);
    } else {  // s==n-1
        if (a0[s] == 0 && lmmp_cmp_(a0, a1, s) < 0) {
            lmmp_sub_n_(asm1, a1, a0, s);
            asm1[s] = 0;
        } else
            asm1[s] = a0[s] - lmmp_sub_n_(asm1, a0, a1, s);
    }

    lmmp_sqr_(vm1, asm1, n);

#undef asm1
#define v0 dst
#define vinf (dst + 2 * n)

    lmmp_sqr_(v0, a0, n);

    lmmp_sqr_(vinf, a1, s);

    cy = lmmp_add_n_(dst + 2 * n, v0 + n, vinf, n);
    cy2 = cy + lmmp_add_n_(dst + n, dst + 2 * n, v0, n);
    cy += lmmp_add_(dst + 2 * n, dst + 2 * n, n, vinf + n, s + s - n);

    cy -= lmmp_sub_n_(dst + n, dst + n, vm1, 2 * n);

    // no overflow.
    lmmp_inc_1(dst + 2 * n, cy2);

    if (cy < 0)
        lmmp_dec(dst + 3 * n);
    else
        lmmp_inc_1(dst + 3 * n, cy);
    TEMP_S_FREE;
}

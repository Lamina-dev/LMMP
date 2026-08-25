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
#include "../../../include/lmmp/impl/toom_interp.h"
#include "../../../include/lmmp/lmmpn.h"


#if LMMP_MPARAM_STATIC_MUL_TOOM33_THRESHOLD < LMMP_MPARAM_STATIC_MUL_TOOM44_THRESHOLD
#define lmmp_sqr_(dst, numa, n)                 \
    if ((n) < MUL_TOOM22_THRESHOLD)             \
        lmmp_sqr_basecase_((dst), (numa), (n)); \
    else if ((n) < MUL_TOOM33_THRESHOLD)        \
        lmmp_sqr_toom2_((dst), (numa), (n));    \
    else                                        \
        lmmp_sqr_toom3_((dst), (numa), (n))
#endif

/*
Evaluate in: -1, 0, +1, +2, +inf

  <-s--><--n--><--n-->
  |-a2-|--a1--|--a0--|

v0  =  a0         *^2 #   A(0)^2
v1  = (a0+ a1+ a2)*^2 #   A(1)^2    ah  <= 2
vm1 = (a0- a1+ a2)*^2 #  A(-1)^2   |ah| <= 1
v2  = (a0+2a1+4a2)*^2 #   A(2)^2    ah  <= 6
vinf=          a2 *^2 # A(inf)^2
*/

void lmmp_sqr_toom3_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa != NULL);
    lmmp_param_assert(dst != NULL);
    TEMP_S_DECL;
    mp_size_t n = (na + 2) / 3, s = na - 2 * n;
    mp_limb_t cy, cy2, vinf0, am1h;
    mp_limb_t* restrict tp = SALLOC_TYPE(4 * n + 4, mp_limb_t);

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)

#define v0 dst               //[dst,2*n]
#define v1 (dst + 2 * n)     //[dst+2*n,2*n+1]
#define vinf (dst + 4 * n)   //[dst+4*n,s+t]
#define vm1 tp               //[tp,2*n+1]
#define v2 (tp + 2 * n + 2)  //[tp+2*n+2,2*n+1]

#define am1 (dst)  //[dst,n]
#define ap1 tp     //[tp,n+1]
#define ap2 ap1    // same space

    // ap1, am1
    cy = lmmp_add_(ap1, a0, n, a2, s);
    if (cy == 0 && lmmp_cmp_(ap1, a1, n) < 0) {
        cy = lmmp_add_n_sub_n_(ap1, am1, a1, ap1, n);
        ap1[n] = cy >> 1;
        am1h = 0;
    } else {
        cy2 = lmmp_add_n_sub_n_(ap1, am1, ap1, a1, n);
        ap1[n] = cy + (cy2 >> 1);
        am1h = cy - (cy2 & 1);
    }

    // vinf
    lmmp_sqr_(vinf, a2, s);
    vinf0 = vinf[0];  // overlap with v1
    cy = vinf[1];     // overlap with v1

    // v1
    lmmp_sqr_(v1, ap1, n + 1);
    vinf[1] = cy;  // restore, since v1[2*n+1]==0.

    // ap2
    cy = lmmp_addshl1_n_(ap2, a1, a2, s);
    if (s != n)
        cy = lmmp_add_1_(ap2 + s, a1 + s, n - s, cy);
    cy = 2 * cy + lmmp_addshl1_n_(ap2, a0, ap2, n);
    ap2[n] = cy;

    // v2
    lmmp_sqr_(v2, ap2, n + 1);

    // vm1
    lmmp_sqr_(vm1, am1, n);
    if (am1h)
        am1h += lmmp_addshl1_n_(vm1 + n, vm1 + n, am1, n);
    vm1[2 * n] = am1h;

    // v0
    lmmp_sqr_(v0, a0, n);

    lmmp_toom_interp5_(dst, v2, vm1, n, s + s, 0, vinf0);
    TEMP_S_FREE;
}

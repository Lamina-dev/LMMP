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


#if MUL_TOOM33_THRESHOLD < MUL_TOOM44_THRESHOLD
#define lmmp_mul_n_(dst, numa, numb, n)                      \
    if ((n) < MUL_TOOM22_THRESHOLD)                          \
        lmmp_mul_basecase_((dst), (numa), (n), (numb), (n)); \
    else if ((n) < MUL_TOOM33_THRESHOLD)                     \
        lmmp_mul_toom22_((dst), (numa), (n), (numb), (n));   \
    else                                                     \
        lmmp_mul_toom33_((dst), (numa), (n), (numb), (n))
#endif

/*
Evaluate in: -1, 0, +1, +2, +inf

  <-s--><--n--><--n-->
  |-a2-|--a1--|--a0--|
   |b2-|--b1--|--b0--|
   <-t-><--n--><--n-->

v0  =  a0         * b0          #   A(0)*B(0)
v1  = (a0+ a1+ a2)*(b0+ b1+ b2) #   A(1)*B(1)      ah  <= 2  bh <= 2
vm1 = (a0- a1+ a2)*(b0- b1+ b2) #  A(-1)*B(-1)    |ah| <= 1  bh <= 1
v2  = (a0+2a1+4a2)*(b0+2b1+4b2) #   A(2)*B(2)      ah  <= 6  bh <= 6
vinf=          a2 *         b2  # A(inf)*B(inf)
*/

void lmmp_mul_toom33_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(nb >= 26);
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(4 * na <= 5 * nb);
    TEMP_S_DECL;
    mp_size_t n = (na + 2) / 3, s = na - 2 * n, t = nb - 2 * n;
    int vm1_neg;
    mp_limb_t cy, cy2, vinf0, am1h, bm1h;
    mp_limb_t* restrict tp = SALLOC_TYPE(4 * n + 4, mp_limb_t);

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define b0 numb
#define b1 (numb + n)
#define b2 (numb + 2 * n)

#define v0 dst               //[dst,2*n]
#define v1 (dst + 2 * n)     //[dst+2*n,2*n+1]
#define vinf (dst + 4 * n)   //[dst+4*n,s+t]
#define vm1 tp               //[tp,2*n+1]
#define v2 (tp + 2 * n + 2)  //[tp+2*n+2,2*n+1]

#define bm1 dst           //[dst,n]
#define am1 (dst + n)     //[dst+n,n]
#define ap1 tp            //[tp,n+1]
#define bp1 (tp + n + 1)  //[tp+n+1,n+1]
#define ap2 ap1           // same space
#define bp2 bp1           // same space

    // ap1, am1
    cy = lmmp_add_(ap1, a0, n, a2, s);
    if (cy == 0 && lmmp_cmp_(ap1, a1, n) < 0) {
        cy = lmmp_add_n_sub_n_(ap1, am1, a1, ap1, n);
        ap1[n] = cy >> 1;
        am1h = 0;
        vm1_neg = 1;
    } else {
        cy2 = lmmp_add_n_sub_n_(ap1, am1, ap1, a1, n);
        ap1[n] = cy + (cy2 >> 1);
        am1h = cy - (cy2 & 1);
        vm1_neg = 0;
    }

    // bp1, bm1
    cy = lmmp_add_(bp1, b0, n, b2, t);
    if (cy == 0 && lmmp_cmp_(bp1, b1, n) < 0) {
        cy = lmmp_add_n_sub_n_(bp1, bm1, b1, bp1, n);
        bp1[n] = cy >> 1;
        bm1h = 0;
        vm1_neg ^= 1;
    } else {
        cy2 = lmmp_add_n_sub_n_(bp1, bm1, bp1, b1, n);
        bp1[n] = cy + (cy2 >> 1);
        bm1h = cy - (cy2 & 1);
    }

    // vinf
    if (s > t)
        lmmp_mul_(vinf, a2, s, b2, t);
    else
        lmmp_mul_n_(vinf, a2, b2, s);
    vinf0 = vinf[0];  // overlap with v1
    cy = vinf[1];     // overlap with v1

    // v1
    lmmp_mul_n_(v1, ap1, bp1, n + 1);
    vinf[1] = cy;  // restore, since v1[2*n+1]==0.

    // ap2
    cy = lmmp_addshl1_n_(ap2, a1, a2, s);
    if (s != n)
        cy = lmmp_add_1_(ap2 + s, a1 + s, n - s, cy);
    cy = 2 * cy + lmmp_addshl1_n_(ap2, a0, ap2, n);
    ap2[n] = cy;

    // bp2
    cy = lmmp_addshl1_n_(bp2, b1, b2, t);
    if (t != n)
        cy = lmmp_add_1_(bp2 + t, b1 + t, n - t, cy);
    cy = 2 * cy + lmmp_addshl1_n_(bp2, b0, bp2, n);
    bp2[n] = cy;

    // v2
    lmmp_mul_n_(v2, ap2, bp2, n + 1);

    // vm1
    lmmp_mul_n_(vm1, am1, bm1, n);
    cy = 0;
    if (am1h)
        cy = bm1h + lmmp_add_n_(vm1 + n, vm1 + n, bm1, n);
    if (bm1h)
        cy += lmmp_add_n_(vm1 + n, vm1 + n, am1, n);
    vm1[2 * n] = cy;

    // v0
    lmmp_mul_n_(v0, a0, b0, n);

    lmmp_toom_interp5_(dst, v2, vm1, n, s + t, vm1_neg, vinf0);
    TEMP_S_FREE;
}

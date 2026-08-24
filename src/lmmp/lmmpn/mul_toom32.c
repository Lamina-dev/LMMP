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


#if LMMP_MPARAM_STATIC_MUL_TOOM33_THRESHOLD < LMMP_MPARAM_STATIC_MUL_TOOM44_THRESHOLD
#define lmmp_mul_n_(dst, numa, numb, n)                      \
    if ((n) < MUL_TOOM22_THRESHOLD)                          \
        lmmp_mul_basecase_((dst), (numa), (n), (numb), (n)); \
    else if ((n) < MUL_TOOM33_THRESHOLD)                     \
        lmmp_mul_toom22_((dst), (numa), (n), (numb), (n));   \
    else                                                     \
        lmmp_mul_toom33_((dst), (numa), (n), (numb), (n))
#endif

/*
Evaluate in: -1, 0, +1, +inf

<-s-><--n--><--n-->
|a2-|---a1-|---a0-|
      |-b1-|---b0-|
      <-t--><--n-->

v0  =  a0       * b0     #   A(0)*B(0)
v1  = (a0+a1+a2)*(b0+b1) #   A(1)*B(1)      ah  <= 2  bh <= 1
vm1 = (a0-a1+a2)*(b0-b1) #  A(-1)*B(-1)    |ah| <= 1  bh = 0
vinf=        a2 *    b1  # A(inf)*B(inf)
*/

void lmmp_mul_toom32_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(nb >= 12);
    lmmp_param_assert(4 * na >= 5 * nb);
    lmmp_param_assert(5 * na <= 9 * nb);
    TEMP_S_DECL;
    mp_size_t n = 1 + (2 * na >= 3 * nb ? (na - 1) / 3 : (nb - 1) >> 1), s = na - 2 * n, t = nb - n;
    int vm1_neg;
    mp_limb_t cy, hi;
    mp_limb_t* restrict tp = SALLOC_TYPE(4 * n + 2, mp_limb_t);

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define b0 numb
#define b1 (numb + n)
    // nb>=12, so that s+t>=n+2
#define bm1 (dst)              //[dst,n]
#define bp1 (dst + n)          //[dst+n,n+1]
#define ap1 (dst + 2 * n + 1)  //[dst+2*n+1,n+1]
#define am1 (dst + 3 * n + 2)  //[dst+3*n+2,n]:hi
#define v1 (tp)                //[tp,2*n+1]
#define vm1 (tp + 2 * n + 1)   //[tp+2*n+1,2*n+1]
#define r0 (dst)
#define r1 (dst + n)
#define r2 (dst + 2 * n)
#define r3 (dst + 3 * n)

    // ap1=a0+a1+a3, am1=a0-a1+a3
    ap1[n] = lmmp_add_(ap1, a0, n, a2, s);
    if (ap1[n] == 0 && lmmp_cmp_(ap1, a1, n) < 0) {
        ap1[n] = lmmp_add_n_sub_n_(ap1, am1, a1, ap1, n) >> 1;
        hi = 0;
        vm1_neg = 1;
    } else {
        cy = lmmp_add_n_sub_n_(ap1, am1, ap1, a1, n);
        hi = ap1[n] - (cy & 1);
        ap1[n] += (cy >> 1);
        vm1_neg = 0;
    }

    // bp1=b0+b1, bm1=b0-b1
    if (t == n) {
        if (lmmp_cmp_(b0, b1, n) < 0) {
            bp1[n] = lmmp_add_n_sub_n_(bp1, bm1, b1, b0, n) >> 1;
            vm1_neg ^= 1;
        } else {
            bp1[n] = lmmp_add_n_sub_n_(bp1, bm1, b0, b1, n) >> 1;
        }
    } else {
        if (lmmp_zero_q_(b0 + t, n - t) && lmmp_cmp_(b0, b1, t) < 0) {
            cy = lmmp_add_n_sub_n_(bp1, bm1, b1, b0, t);
            lmmp_zero(bm1 + t, n - t);
            vm1_neg ^= 1;
        } else {
            cy = lmmp_add_n_sub_n_(bp1, bm1, b0, b1, t);
            lmmp_sub_1_(bm1 + t, b0 + t, n - t, cy & 1);
        }
        bp1[n] = lmmp_add_1_(bp1 + t, b0 + t, n - t, cy >> 1);
    }

    // v1=ap1*bp1
    lmmp_mul_n_(v1, ap1, bp1, n + 1);

    // vm=am1*bm1
    lmmp_mul_n_(vm1, am1, bm1, n);
    if (hi)
        hi = lmmp_add_n_(vm1 + n, vm1 + n, bm1, n);
    vm1[2 * n] = hi;

    // r0=a0*b0
    // r3=a2*b1
    lmmp_mul_n_(r0, a0, b0, n);
    if (s > t)
        lmmp_mul_(r3, a2, s, b1, t);
    else
        lmmp_mul_(r3, b1, t, a2, s);

    // v1=(v1+vm1)/2, (=a0*b0+a2*b0+a1*b1)
    // vm1=v1-vm1, (=a1*b0+a0*b1+a2*b1)
    if (vm1_neg) {
        lmmp_shr1sub_n_(v1, v1, vm1, 2 * n + 1);
        lmmp_add_n_(vm1, v1, vm1, 2 * n + 1);
    } else {
        lmmp_shr1add_n_(v1, v1, vm1, 2 * n + 1);
        lmmp_sub_n_(vm1, v1, vm1, 2 * n + 1);
    }

    // vm1-=r3, (=r1)
    // v1-=r0, (=r2)
    lmmp_sub_(vm1, vm1, 2 * n + 1, r3, s + t);
    v1[2 * n] -= lmmp_sub_n_(v1, v1, r0, 2 * n);

    // r=r0+vm1*B+v1*B^2+r3*B^4
    cy = vm1[2 * n] + lmmp_add_(r1, vm1, 2 * n, r1, n);
    lmmp_add_(r2, r2, n + s + t, v1, 2 * n + 1);
    lmmp_inc_1(r3, cy);
    TEMP_S_FREE;
}

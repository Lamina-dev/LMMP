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

#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/toom_interp.h"


#define lmmp_sqr_(dst, numa, n)                 \
    if ((n) < MUL_TOOM22_THRESHOLD)             \
        lmmp_sqr_basecase_((dst), (numa), (n)); \
    else if ((n) < MUL_TOOM33_THRESHOLD)        \
        lmmp_sqr_toom2_((dst), (numa), (n));    \
    else if ((n < MUL_TOOM44_THRESHOLD))        \
        lmmp_sqr_toom3_((dst), (numa), (n));    \
    else                                        \
        lmmp_sqr_toom4_((dst), (numa), (n))

/* 
Evaluate in: -1, -1/2, 0, +1/2, +1, +2, +inf

  <-s--><--n--><--n--><--n-->
  |-a3-|--a2--|--a1--|--a0--|

  v0  =   a0             ^2 #    A(0)^2
  v1  = ( a0+ a1+ a2+ a3)^2 #    A(1)^2   ah  <= 3
  vm1 = ( a0- a1+ a2- a3)^2 #   A(-1)^2  |ah| <= 1
  v2  = ( a0+2a1+4a2+8a3)^2 #    A(2)^2   ah  <= 14
  vh  = (8a0+4a1+2a2+ a3)^2 #  A(1/2)^2   ah  <= 14
  vmh = (8a0-4a1+2a2- a3)^2 # A(-1/2)^2  -4<=ah<=9
  vinf=               a3 ^2 #  A(inf)^2
*/

void lmmp_sqr_toom4_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(numa != NULL);
    mp_size_t n, s;
    mp_limb_t cy;

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define a3 (numa + 3 * n)

    n = (na + 3) >> 2;
    TEMP_S_DECL;
    mp_ptr restrict scratch = SALLOC_TYPE(10 * n + 10, mp_limb_t);

    s = na - 3 * n;

    lmmp_debug_assert(0 < s && s <= n);

    /* NOTE: The multiplications to v2, vm2, vh and vm1 overwrites the
     * following limb, so these must be computed in order, and we need a
     * one limb gap to tp. */
#define v0 dst                     /* 2n */
#define v1 (dst + 2 * n)           /* 2n+1 */
#define vinf (dst + 6 * n)         /* s+t */
#define v2 scratch                 /* 2n+1 */
#define vm2 (scratch + 2 * n + 1)  /* 2n+1 */
#define vh (scratch + 4 * n + 2)   /* 2n+1 */
#define vm1 (scratch + 6 * n + 3)  /* 2n+1 */
#define tp (scratch + 8 * n + 5)

    /* No overlap with v1 */
#define apx dst               /* n+1 */
#define amx (dst + 4 * n + 2) /* n+1 */

    /* Compute apx = a0 + 2 a1 + 4 a2 + 8 a3 and amx = a0 - 2 a1 + 4 a2 - 8 a3.  */
    lmmp_toom_eval_dgr3_pm2_(apx, amx, numa, n, s, tp);

    lmmp_sqr_(v2, apx, n + 1);  /* v2,  2n+1 limbs */
    lmmp_sqr_(vm2, amx, n + 1); /* vm2,  2n+1 limbs */

    /* Compute apx = 8 a0 + 4 a1 + 2 a2 + a3 = (((2*a0 + a1) * 2 + a2) * 2 + a3 */
    cy = lmmp_addshl1_n_(apx, a1, a0, n);
    cy = 2 * cy + lmmp_addshl1_n_(apx, a2, apx, n);
    if (s < n) {
        mp_limb_t cy2;
        cy2 = lmmp_addshl1_n_(apx, a3, apx, s);
        apx[n] = 2 * cy + lmmp_shl_(apx + s, apx + s, n - s, 1);
        lmmp_inc_1(apx + s, cy2);
    } else
        apx[n] = 2 * cy + lmmp_addshl1_n_(apx, a3, apx, n);

    lmmp_debug_assert(apx[n] < 15);

    lmmp_sqr_(vh, apx, n + 1); /* vh,  2n+1 limbs */

    /* Compute apx = a0 + a1 + a2 + a3 and amx = a0 - a1 + a2 - a3.  */
    lmmp_toom_eval_dgr3_pm1_(apx, amx, numa, n, s, tp);

    lmmp_sqr_(v1, apx, n + 1);  /* v1,  2n+1 limbs */
    lmmp_sqr_(vm1, amx, n + 1); /* vm1,  2n+1 limbs */

    lmmp_sqr_(v0, a0, n);
    lmmp_sqr_(vinf, a3, s); /* vinf, 2s limbs */

    lmmp_toom_interp7_(dst, n, (enum toom7_flags)0, vm2, vm1, v2, vh, 2 * s, tp);
    TEMP_S_FREE;
}

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


#if MUL_TOOM44_THRESHOLD < MUL_FFT_THRESHOLD
#define lmmp_mul_n_(dst, numa, numb, n)                      \
    if ((n) < MUL_TOOM22_THRESHOLD)                          \
        lmmp_mul_basecase_((dst), (numa), (n), (numb), (n)); \
    else if ((n) < MUL_TOOM33_THRESHOLD)                     \
        lmmp_mul_toom22_((dst), (numa), (n), (numb), (n));   \
    else if ((n) < MUL_TOOM44_THRESHOLD)                     \
        lmmp_mul_toom33_((dst), (numa), (n), (numb), (n));   \
    else                                                     \
        lmmp_mul_toom44_((dst), (numa), (n), (numb), (n))
#endif

/* 
Evaluate in: 0, +1, -1, +2, -2, 1/2, +inf

  <-s--><--n--><--n--><--n-->
  |-a3-|--a2--|--a1--|--a0--|
   |b3-|--b2--|--b1--|--b0--|
   <-t-><--n--><--n--><--n-->

  v0  =   a0             *  b0              #    A(0)*B(0)
  v1  = ( a0+ a1+ a2+ a3)*( b0+ b1+ b2+ b3) #    A(1)*B(1)      ah  <= 3   bh  <= 3
  vm1 = ( a0- a1+ a2- a3)*( b0- b1+ b2- b3) #   A(-1)*B(-1)    |ah| <= 1  |bh| <= 1
  v2  = ( a0+2a1+4a2+8a3)*( b0+2b1+4b2+8b3) #    A(2)*B(2)      ah  <= 14  bh  <= 14
  vm2 = ( a0-2a1+4a2-8a3)*( b0-2b1+4b2-8b3) #    A(2)*B(2)      ah  <= 9  |bh| <= 9
  vh  = (8a0+4a1+2a2+ a3)*(8b0+4b1+2b2+ b3) #  A(1/2)*B(1/2)    ah  <= 14  bh  <= 14
  vinf=               a3 *          b2      #  A(inf)*B(inf)
*/

void lmmp_mul_toom44_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(4 * na <= 5 * nb);
    mp_size_t n, s, t;
    mp_limb_t cy;
    enum toom7_flags flags;

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define a3 (numa + 3 * n)
#define b0 numb
#define b1 (numb + n)
#define b2 (numb + 2 * n)
#define b3 (numb + 3 * n)

    lmmp_debug_assert(na >= nb);

    n = (na + 3) >> 2;
    TEMP_S_DECL;
    mp_ptr restrict scratch = SALLOC_TYPE(10 * n + 10, mp_limb_t);

    s = na - 3 * n;
    t = nb - 3 * n;

    lmmp_debug_assert(0 < s && s <= n);
    lmmp_debug_assert(0 < t && t <= n);
    lmmp_debug_assert(s >= t);

    /* NOTE: The multiplications to v2, vm2, vh and vm1 overwrites the
     * following limb, so these must be computed in order, and we need a
     * one limb gap to tp. */
#define v0 dst                     /* 2n   */
#define v1 (dst + 2 * n)           /* 2n+1 */
#define vinf (dst + 6 * n)         /* s+t  */
#define v2 scratch                 /* 2n+1 */
#define vm2 (scratch + 2 * n + 1)  /* 2n+1 */
#define vh (scratch + 4 * n + 2)   /* 2n+1 */
#define vm1 (scratch + 6 * n + 3)  /* 2n+1 */
#define tp (scratch + 8 * n + 5)

    /* apx and bpx must not overlap with v1 */
#define apx dst               /* n+1 */
#define amx (dst + n + 1)     /* n+1 */
#define bmx (dst + 2 * n + 2) /* n+1 */
#define bpx (dst + 4 * n + 2) /* n+1 */

    /* Total scratch need: 8*n + 5 + scratch for recursive calls. This
       gives roughly 32 n/3 + log term. */

    /* Compute apx = a0 + 2 a1 + 4 a2 + 8 a3 and amx = a0 - 2 a1 + 4 a2 - 8 a3.  */
    flags = (enum toom7_flags)(toom7_w1_neg & lmmp_toom_eval_dgr3_pm2_(apx, amx, numa, n, s, tp));

    /* Compute bpx = b0 + 2 b1 + 4 b2 + 8 b3 and bmx = b0 - 2 b1 + 4 b2 - 8 b3.  */
    flags = (enum toom7_flags)(flags ^ (toom7_w1_neg & lmmp_toom_eval_dgr3_pm2_(bpx, bmx, numb, n, t, tp)));

    lmmp_mul_n_(v2, apx, bpx, n + 1);  /* v2,  2n+1 limbs */
    lmmp_mul_n_(vm2, amx, bmx, n + 1); /* vm2,  2n+1 limbs */

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


    /* Compute bpx = 8 b0 + 4 b1 + 2 b2 + b3 = (((2*b0 + b1) * 2 + b2) * 2 + b3 */

    cy = lmmp_addshl1_n_(bpx, b1, b0, n);
    cy = 2 * cy + lmmp_addshl1_n_(bpx, b2, bpx, n);
    if (t < n) {
        mp_limb_t cy2;
        cy2 = lmmp_addshl1_n_(bpx, b3, bpx, t);
        bpx[n] = 2 * cy + lmmp_shl_(bpx + t, bpx + t, n - t, 1);
        lmmp_inc_1(bpx + t, cy2);
    } else
        bpx[n] = 2 * cy + lmmp_addshl1_n_(bpx, b3, bpx, n);

    lmmp_debug_assert(apx[n] < 15);
    lmmp_debug_assert(bpx[n] < 15);

    lmmp_mul_n_(vh, apx, bpx, n + 1); /* vh,  2n+1 limbs */

    /* Compute apx = a0 + a1 + a2 + a3 and amx = a0 - a1 + a2 - a3.  */
    flags = (enum toom7_flags)(flags | (toom7_w3_neg & lmmp_toom_eval_dgr3_pm1_(apx, amx, numa, n, s, tp)));

    /* Compute bpx = b0 + b1 + b2 + b3 and bmx = b0 - b1 + b2 - b3.  */
    flags = (enum toom7_flags)(flags ^ (toom7_w3_neg & lmmp_toom_eval_dgr3_pm1_(bpx, bmx, numb, n, t, tp)));

    lmmp_mul_n_(vm1, amx, bmx, n + 1); /* vm1,  2n+1 limbs */
    /* Clobbers amx, bmx. */
    lmmp_mul_n_(v1, apx, bpx, n + 1); /* v1,  2n+1 limbs */

    lmmp_mul_n_(v0, a0, b0, n);
    if (s > t)
        lmmp_mul_(vinf, a3, s, b3, t);
    else
        lmmp_mul_n_(vinf, a3, b3, s); 

    lmmp_toom_interp7_(dst, n, flags, vm2, vm1, v2, vh, s + t, tp);

    TEMP_S_FREE;
}
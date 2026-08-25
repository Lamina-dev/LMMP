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


#if LMMP_MPARAM_STATIC_MUL_TOOM44_THRESHOLD < LMMP_MPARAM_STATIC_MUL_FFT_THRESHOLD
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
Evaluate in: -2, -1, 0, +1, +2, +inf

  <-s-><--n--><--n--><--n--><--n-->
  |a4-|--a3--|--a2--|--a1--|--a0--|
                        |b1|--b0--|
                        <t-><--n-->

  v0  =  a0                  * b0      #   A(0)*B(0)
  v1  = (a0+ a1+ a2+ a3+  a4)*(b0+ b1) #   A(1)*B(1)      ah  <= 4   bh <= 1
  vm1 = (a0- a1+ a2- a3+  a4)*(b0- b1) #  A(-1)*B(-1)    |ah| <= 2   bh  = 0
  v2  = (a0+2a1+4a2+8a3+16a4)*(b0+2b1) #   A(2)*B(2)      ah  <= 30  bh <= 2
  vm2 = (a0-2a1+4a2-8a3+16a4)*(b0-2b1) #  A(-2)*B(-2)    |ah| <= 20 |bh|<= 1
  vinf=                   a4 *     b1  # A(inf)*B(inf)

  Some slight optimization in evaluation are taken from the paper:
  "Towards Optimal Toom-Cook Multiplication for Univariate and
  Multivariate Polynomials in Characteristic 2 and 0."
*/

void lmmp_mul_toom52_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(9 * na >= 20 * nb);
    lmmp_param_assert(3 * nb >= na);
    mp_size_t n, s, t;
    enum toom6_flags flags;

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define a3 (numa + 3 * n)
#define a4 (numa + 4 * n)
#define b0 numb
#define b1 (numb + n)

    n = 1 + (2 * na >= 5 * nb ? (na - 1) / (mp_size_t)5 : (nb - 1) >> 1);
    TEMP_S_DECL;
    mp_ptr restrict scratch = SALLOC_TYPE(6 * n + 6, mp_limb_t);

    s = na - 4 * n;
    t = nb - n;

    lmmp_debug_assert(0 < s && s <= n);
    lmmp_debug_assert(0 < t && t <= n);

    /* Ensures that 5 values of n+1 limbs each fits in the product area.
       Borderline cases are na = 32, nb = 8, n = 7, and na = 36, bn = 9,
       n = 8. */
    lmmp_debug_assert(s + t >= 5);

#define v0 dst                      /* 2n   */
#define vm1 (scratch)               /* 2n+1 */
#define v1 (dst + 2 * n)            /* 2n+1 */
#define vm2 (scratch + 2 * n + 1)   /* 2n+1 */
#define v2 (scratch + 4 * n + 2)    /* 2n+1 */
#define vinf (dst + 5 * n)          /* s+t  */
#define bs1 dst                     /* n+1  */
#define bsm1 (scratch + 2 * n + 2)  /* n    */
#define asm1 (scratch + 3 * n + 3)  /* n+1  */
#define asm2 (scratch + 4 * n + 4)  /* n+1  */
#define bsm2 (dst + n + 1)          /* n+1  */
#define bs2 (dst + 2 * n + 2)       /* n+1  */
#define as2 (dst + 3 * n + 3)       /* n+1  */
#define as1 (dst + 4 * n + 4)       /* n+1  */


#define a0a2 scratch
#define a1a3 asm1

    /* Compute as2 and asm2.  */
    flags = (enum toom6_flags)(toom6_vm2_neg & lmmp_toom_eval_pm2_(as2, asm2, 4, numa, n, s, a1a3));

    /* Compute bs1 and bsm1.  */
    if (t == n) {
        mp_limb_t cy;
        if (lmmp_cmp_(b0, b1, n) < 0) {
            cy = lmmp_add_n_sub_n_(bs1, bsm1, b1, b0, n);
            flags = (enum toom6_flags)(flags ^ toom6_vm1_neg);
        } else {
            cy = lmmp_add_n_sub_n_(bs1, bsm1, b0, b1, n);
        }
        bs1[n] = cy >> 1;
    } else {
        bs1[n] = lmmp_add_(bs1, b0, n, b1, t);
        if (lmmp_zero_q_(b0 + t, n - t) && lmmp_cmp_(b0, b1, t) < 0) {
            lmmp_sub_n_(bsm1, b1, b0, t);
            lmmp_zero(bsm1 + t, n - t);
            flags = (enum toom6_flags)(flags ^ toom6_vm1_neg);
        } else {
            lmmp_sub_(bsm1, b0, n, b1, t);
        }
    }

    /* Compute bs2 and bsm2, recycling bs1 and bsm1. bs2=bs1+b1; bsm2=bsm1-b1  */
    lmmp_add_(bs2, bs1, n + 1, b1, t);
    if (flags & toom6_vm1_neg) {
        bsm2[n] = lmmp_add_(bsm2, bsm1, n, b1, t);
        flags = (enum toom6_flags)(flags ^ toom6_vm2_neg);
    } else {
        bsm2[n] = 0;
        if (t == n) {
            if (lmmp_cmp_(bsm1, b1, n) < 0) {
                lmmp_sub_n_(bsm2, b1, bsm1, n);
                flags = (enum toom6_flags)(flags ^ toom6_vm2_neg);
            } else {
                lmmp_sub_n_(bsm2, bsm1, b1, n);
            }
        } else {
            if (lmmp_zero_q_(bsm1 + t, n - t) && lmmp_cmp_(bsm1, b1, t) < 0) {
                lmmp_sub_n_(bsm2, b1, bsm1, t);
                lmmp_zero(bsm2 + t, n - t);
                flags = (enum toom6_flags)(flags ^ toom6_vm2_neg);
            } else {
                lmmp_sub_(bsm2, bsm1, n, b1, t);
            }
        }
    }

    /* Compute as1 and asm1.  */
    flags = (enum toom6_flags)(flags ^ (toom6_vm1_neg & lmmp_toom_eval_pm1_(as1, asm1, 4, numa, n, s, a0a2)));

    lmmp_debug_assert(as1[n] <= 4);
    lmmp_debug_assert(bs1[n] <= 1);
    lmmp_debug_assert(asm1[n] <= 2);
    /*   lmmp_debug_assert (bsm1[n] <= 1); */
    lmmp_debug_assert(as2[n] <= 30);
    lmmp_debug_assert(bs2[n] <= 2);
    lmmp_debug_assert(asm2[n] <= 20);
    lmmp_debug_assert(bsm2[n] <= 1);

    /* vm1, 2n+1 limbs */
    lmmp_mul_(vm1, asm1, n + 1, bsm1, n); /* W4 */

    /* vm2, 2n+1 limbs */
    lmmp_mul_n_(vm2, asm2, bsm2, n + 1); /* W2 */

    /* v2, 2n+1 limbs */
    lmmp_mul_n_(v2, as2, bs2, n + 1); /* W1 */

    /* v1, 2n+1 limbs */
    lmmp_mul_n_(v1, as1, bs1, n + 1); /* W3 */

    /* vinf, s+t limbs */ /* W0 */
    if (s > t)
        lmmp_mul_(vinf, a4, s, b1, t);
    else
        lmmp_mul_(vinf, b1, t, a4, s);

    /* v0, 2n limbs */
    lmmp_mul_n_(v0, numa, numb, n); /* W5 */

    lmmp_toom_interp6_(dst, n, flags, vm1, vm2, v2, t + s);
    TEMP_S_FREE;
#undef v0
#undef vm1
#undef v1
#undef vm2
#undef v2
#undef vinf
#undef bs1
#undef bs2
#undef bsm1
#undef bsm2
#undef asm1
#undef asm2
#undef as1
#undef as2
#undef a0a2
#undef b0b2
#undef a1a3
#undef a0
#undef a1
#undef a2
#undef a3
#undef b0
#undef b1
#undef b2
}

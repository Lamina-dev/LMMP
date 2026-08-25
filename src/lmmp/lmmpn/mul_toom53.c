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
Evaluate in: 0, +1, -1, +2, -2, 1/2, +inf

  <-s-><--n--><--n--><--n--><--n-->
  |a4-|--a3--|--a2--|--a1--|--a0--|
               |--b2|--b1--|--b0--|
               <-t--><--n--><--n-->

  v0  =    a0                  *  b0          #    A(0)*B(0)
  v1  = (  a0+ a1+ a2+ a3+  a4)*( b0+ b1+ b2) #    A(1)*B(1)      ah  <= 4   bh <= 2
  vm1 = (  a0- a1+ a2- a3+  a4)*( b0- b1+ b2) #   A(-1)*B(-1)    |ah| <= 2   bh <= 1
  v2  = (  a0+2a1+4a2+8a3+16a4)*( b0+2b1+4b2) #    A(2)*B(2)      ah  <= 30  bh <= 6
  vm2 = (  a0-2a1+4a2-8a3+16a4)*( b0-2b1+4b2) #    A(2)*B(2)     -9<=ah<=20 -1<=bh<=4
  vh  = (16a0+8a1+4a2+2a3+  a4)*(4b0+2b1+ b2) #  A(1/2)*B(1/2)    ah  <= 30  bh <= 6
  vinf=                     a4 *          b2  #  A(inf)*B(inf)
*/

void lmmp_mul_toom53_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(9 * na <= 20 * nb);
    lmmp_param_assert(5 * nb <= 3 * na);
    mp_size_t n, s, t;
    mp_limb_t cy;
    mp_ptr gp;
    mp_ptr as1, asm1, as2, asm2, ash;
    mp_ptr bs1, bsm1, bs2, bsm2, bsh;
    mp_ptr tmp;
    enum toom7_flags flags;
    TEMP_S_DECL;

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define a3 (numa + 3 * n)
#define a4 (numa + 4 * n)
#define b0 numb
#define b1 (numb + n)
#define b2 (numb + 2 * n)

    n = 1 + (3 * na >= 5 * nb ? (na - 1) / (mp_size_t)5 : (nb - 1) / (mp_size_t)3);
    mp_ptr restrict scratch = SALLOC_TYPE(10 * (n + 1), mp_limb_t);
    s = na - 4 * n;
    t = nb - 2 * n;

    tmp = SALLOC_TYPE(10 * (n + 1), mp_limb_t);
    as1 = tmp;
    tmp += n + 1;
    asm1 = tmp;
    tmp += n + 1;
    as2 = tmp;
    tmp += n + 1;
    asm2 = tmp;
    tmp += n + 1;
    ash = tmp;
    tmp += n + 1;
    bs1 = tmp;
    tmp += n + 1;
    bsm1 = tmp;
    tmp += n + 1;
    bs2 = tmp;
    tmp += n + 1;
    bsm2 = tmp;
    tmp += n + 1;
    bsh = tmp;
    tmp += n + 1;

    gp = dst;

    /* Compute as1 and asm1.  */
    flags = (enum toom7_flags)(toom7_w3_neg & lmmp_toom_eval_pm1_(as1, asm1, 4, numa, n, s, gp));

    /* Compute as2 and asm2. */
    flags = (enum toom7_flags)(flags | (toom7_w1_neg & lmmp_toom_eval_pm2_(as2, asm2, 4, numa, n, s, gp)));

    /* Compute ash = 16 a0 + 8 a1 + 4 a2 + 2 a3 + a4
       = 2*(2*(2*(2*a0 + a1) + a2) + a3) + a4  */
    cy = lmmp_addshl1_n_(ash, a1, a0, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a2, ash, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a3, ash, n);
    if (s < n) {
        mp_limb_t cy2;
        cy2 = lmmp_addshl1_n_(ash, a4, ash, s);
        ash[n] = 2 * cy + lmmp_shl_(ash + s, ash + s, n - s, 1);
        lmmp_inc_1(ash + s, cy2);
    } else
        ash[n] = 2 * cy + lmmp_addshl1_n_(ash, a4, ash, n);


    /* Compute bs1 and bsm1.  */
    bs1[n] = lmmp_add_(bs1, b0, n, b2, t); /* b0 + b2 */
    if (bs1[n] == 0 && lmmp_cmp_(bs1, b1, n) < 0) {
        bs1[n] = lmmp_add_n_sub_n_(bs1, bsm1, b1, bs1, n) >> 1;
        bsm1[n] = 0;
        flags = (enum toom7_flags)(flags ^ toom7_w3_neg);
    } else {
        cy = lmmp_add_n_sub_n_(bs1, bsm1, bs1, b1, n);
        bsm1[n] = bs1[n] - (cy & 1);
        bs1[n] += (cy >> 1);
    }

    /* Compute bs2 and bsm2. */

    cy = lmmp_shl_(gp, b2, t, 2);
    bs2[n] = lmmp_add_(bs2, b0, n, gp, t);
    lmmp_inc_1(bs2 + t, cy);

    gp[n] = lmmp_shl_(gp, b1, n, 1);

    if (lmmp_cmp_(bs2, gp, n + 1) < 0) {
        lmmp_add_n_sub_n_(bs2, bsm2, gp, bs2, n + 1);
        flags = (enum toom7_flags)(flags ^ toom7_w1_neg);
    } else {
        lmmp_add_n_sub_n_(bs2, bsm2, bs2, gp, n + 1);
    }

    /* Compute bsh = 4 b0 + 2 b1 + b2 = 2*(2*b0 + b1)+b2.  */

    cy = lmmp_addshl1_n_(bsh, b1, b0, n);
    if (t < n) {
        mp_limb_t cy2;
        cy2 = lmmp_addshl1_n_(bsh, b2, bsh, t);
        bsh[n] = 2 * cy + lmmp_shl_(bsh + t, bsh + t, n - t, 1);
        lmmp_inc_1(bsh + t, cy2);
    } else
        bsh[n] = 2 * cy + lmmp_addshl1_n_(bsh, b2, bsh, n);

    lmmp_debug_assert(as1[n] <= 4);
    lmmp_debug_assert(bs1[n] <= 2);
    lmmp_debug_assert(asm1[n] <= 2);
    lmmp_debug_assert(bsm1[n] <= 1);
    lmmp_debug_assert(as2[n] <= 30);
    lmmp_debug_assert(bs2[n] <= 6);
    lmmp_debug_assert(asm2[n] <= 20);
    lmmp_debug_assert(bsm2[n] <= 4);
    lmmp_debug_assert(ash[n] <= 30);
    lmmp_debug_assert(bsh[n] <= 6);

#define v0 dst                             /* 2n   */
#define v1 (dst + 2 * n)                   /* 2n+1 */
#define vinf (dst + 6 * n)                 /* s+t  */
#define v2 scratch                         /* 2n+1 */
#define vm2 (scratch + 2 * n + 1)          /* 2n+1 */
#define vh (scratch + 4 * n + 2)           /* 2n+1 */
#define vm1 (scratch + 6 * n + 3)          /* 2n+1 */
#define scratch_out (scratch + 8 * n + 4)  /* 2n+1 */
    /* Total scratch need: 10*n+5 */

    /* Must be in allocation order, as they overwrite one limb beyond
     * 2n+1. */
    lmmp_mul_n_(v2, as2, bs2, n + 1);    /* v2, 2n+1 limbs */
    lmmp_mul_n_(vm2, asm2, bsm2, n + 1); /* vm2, 2n+1 limbs */
    lmmp_mul_n_(vh, ash, bsh, n + 1);    /* vh, 2n+1 limbs */

    /* vm1, 2n+1 limbs */
    vm1[2 * n] = 0;
    lmmp_mul_n_(vm1, asm1, bsm1, n + ((asm1[n] | bsm1[n]) != 0));


    /* v1, 2n+1 limbs */
    v1[2 * n] = 0;
    lmmp_mul_n_(v1, as1, bs1, n + ((as1[n] | bs1[n]) != 0));


    lmmp_mul_n_(v0, a0, b0, n); /* v0, 2n limbs */

    /* vinf, s+t limbs */
    if (s > t)
        lmmp_mul_(vinf, a4, s, b2, t);
    else
        lmmp_mul_(vinf, b2, t, a4, s);

    lmmp_toom_interp7_(dst, n, flags, vm2, vm1, v2, vh, s + t, scratch_out);

    TEMP_S_FREE;
}
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

  <-s-><--n--><--n--><--n-->
  |a3-|---a2-|--a1--|--a0--|
        |-b2-|--b1--|--b0--|
        <-t--><--n--><--n-->

  v0  =  a0             * b0          #   A(0)*B(0)
  v1  = (a0+ a1+ a2+ a3)*(b0+ b1+ b2) #   A(1)*B(1)      ah  <= 3  bh <= 2
  vm1 = (a0- a1+ a2- a3)*(b0- b1+ b2) #  A(-1)*B(-1)    |ah| <= 1 |bh|<= 1
  v2  = (a0+2a1+4a2+8a3)*(b0+2b1+4b2) #   A(2)*B(2)      ah  <= 14 bh <= 6
  vm2 = (a0-2a1+4a2-8a3)*(b0-2b1+4b2) #  A(-2)*B(-2)    |ah| <= 9 |bh|<= 4
  vinf=              a3 *         b2  # A(inf)*B(inf)
*/

void lmmp_mul_toom43_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(4 * na >= 5 * nb);
    lmmp_param_assert(3 * na <= 5 * nb);
    mp_size_t n, s, t;
    enum toom6_flags flags;
    mp_limb_t cy;

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define a3 (numa + 3 * n)
#define b0 numb
#define b1 (numb + n)
#define b2 (numb + 2 * n)

    n = 1 + (3 * na >= 4 * nb ? (na - 1) >> 2 : (nb - 1) / (mp_size_t)3);
    TEMP_S_DECL;
    mp_limb_t* restrict scratch = SALLOC_TYPE(6 * n + 6, mp_limb_t);

    s = na - 3 * n;
    t = nb - 2 * n;

    lmmp_debug_assert(0 < s && s <= n);
    lmmp_debug_assert(0 < t && t <= n);

    /* This is true whenever na >= 25 or nb >= 19, I think. It
       guarantees that we can fit 5 values of size n+1 in the product
       area. */
    lmmp_debug_assert(s + t >= 5);

#define v0 dst                      /* 2n   */
#define vm1 (scratch)               /* 2n+1 */
#define v1 (dst + 2 * n)            /* 2n+1 */
#define vm2 (scratch + 2 * n + 1)   /* 2n+1 */
#define v2 (scratch + 4 * n + 2)    /* 2n+1 */
#define vinf (dst + 5 * n)          /* s+t  */
#define bs1 dst                     /* n+1  */
#define bsm1 (scratch + 2 * n + 2)  /* n+1  */
#define asm1 (scratch + 3 * n + 3)  /* n+1  */
#define asm2 (scratch + 4 * n + 4)  /* n+1  */
#define bsm2 (dst + n + 1)          /* n+1  */
#define bs2 (dst + 2 * n + 2)       /* n+1  */
#define as2 (dst + 3 * n + 3)       /* n+1  */
#define as1 (dst + 4 * n + 4)       /* n+1  */

    /* Total sccratch need is 6 * n + 3 + 1; we allocate one extra
       limb, because products will overwrite 2n+2 limbs. */

#define a0a2 scratch
#define b0b2 scratch
#define a1a3 asm1
#define b1d bsm1

    /* Compute as2 and asm2.  */
    flags = (enum toom6_flags)(toom6_vm2_neg & lmmp_toom_eval_dgr3_pm2_(as2, asm2, numa, n, s, a1a3));

    /* Compute bs2 and bsm2.  */
    b1d[n] = lmmp_shl_(b1d, b1, n, 1);    /*       2b1      */
    cy = lmmp_shl_(b0b2, b2, t, 2);       /*  4b2           */
    cy += lmmp_add_n_(b0b2, b0b2, b0, t); /*  4b2      + b0 */
    if (t != n)
        cy = lmmp_add_1_(b0b2 + t, b0 + t, n - t, cy);
    b0b2[n] = cy;

    if (lmmp_cmp_(b0b2, b1d, n + 1) < 0) {
        lmmp_add_n_sub_n_(bs2, bsm2, b1d, b0b2, n + 1);
        flags = (enum toom6_flags)(flags ^ toom6_vm2_neg);
    } else {
        lmmp_add_n_sub_n_(bs2, bsm2, b0b2, b1d, n + 1);
    }


    /* Compute as1 and asm1.  */
    flags = (enum toom6_flags)(flags ^ (toom6_vm1_neg & lmmp_toom_eval_dgr3_pm1_(as1, asm1, numa, n, s, a0a2)));

    /* Compute bs1 and bsm1.  */
    bsm1[n] = lmmp_add_(bsm1, b0, n, b2, t);
    if (bsm1[n] == 0 && lmmp_cmp_(bsm1, b1, n) < 0) {
        cy = lmmp_add_n_sub_n_(bs1, bsm1, b1, bsm1, n);
        bs1[n] = cy >> 1;
        flags = (enum toom6_flags)(flags ^ toom6_vm1_neg);
    } else {
        cy = lmmp_add_n_sub_n_(bs1, bsm1, bsm1, b1, n);
        bs1[n] = bsm1[n] + (cy >> 1);
        bsm1[n] -= cy & 1;
    }

    lmmp_debug_assert(as1[n] <= 3);
    lmmp_debug_assert(bs1[n] <= 2);
    lmmp_debug_assert(asm1[n] <= 1);
    lmmp_debug_assert(bsm1[n] <= 1);
    lmmp_debug_assert(as2[n] <= 14);
    lmmp_debug_assert(bs2[n] <= 6);
    lmmp_debug_assert(asm2[n] <= 9);
    lmmp_debug_assert(bsm2[n] <= 4);

    /* vm1, 2n+1 limbs */
    lmmp_mul_n_(vm1, asm1, bsm1, n + 1); /* W4 */

    /* vm2, 2n+1 limbs */
    lmmp_mul_n_(vm2, asm2, bsm2, n + 1); /* W2 */

    /* v2, 2n+1 limbs */
    lmmp_mul_n_(v2, as2, bs2, n + 1); /* W1 */

    /* v1, 2n+1 limbs */
    lmmp_mul_n_(v1, as1, bs1, n + 1); /* W3 */

    /* vinf, s+t limbs */ /* W0 */
    if (s > t)
        lmmp_mul_(vinf, a3, s, b2, t);
    else
        lmmp_mul_(vinf, b2, t, a3, s);

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
#undef b1d
#undef a0
#undef a1
#undef a2
#undef a3
#undef b0
#undef b1
#undef b2
}

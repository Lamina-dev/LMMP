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

#include "../../../include/lammp/impl/mparam.h"
#include "../../../include/lammp/impl/toom_interp.h"


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
Evaluate in:
   0, +1, -1, +2, -2, 1/2, +inf

  <-s-><--n--><--n--><--n--><--n--><--n-->
  |a5-|--a4--|--a3--|--a2--|--a1--|--a0--|
                             |-b1-|--b0--|
                             <-t--><--n-->

  v0  =    a0                       *   b0      #    A(0)*B(0)
  v1  = (  a0+  a1+ a2+ a3+  a4+  a5)*( b0+ b1) #    A(1)*B(1)      ah  <= 5   bh <= 1
  vm1 = (  a0-  a1+ a2- a3+  a4-  a5)*( b0- b1) #   A(-1)*B(-1)    |ah| <= 2   bh  = 0
  v2  = (  a0+ 2a1+4a2+8a3+16a4+32a5)*( b0+2b1) #    A(2)*B(2)      ah  <= 62  bh <= 2
  vm2 = (  a0- 2a1+4a2-8a3+16a4-32a5)*( b0-2b1) #   A(-2)*B(-2)    -41<=ah<=20 -1<=bh<=0
  vh  = (32a0+16a1+8a2+4a3+ 2a4+  a5)*(2b0+ b1) #  A(1/2)*B(1/2)    ah  <= 62  bh <= 2
  vinf=                           a5 *      b1  #  A(inf)*B(inf)
*/

void lmmp_mul_toom62_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(na >= 3 * nb);
    lmmp_param_assert(5 * nb >= na);

    mp_size_t n, s, t;
    mp_limb_t cy;
    mp_ptr as1, asm1, as2, asm2, ash;
    mp_ptr bs1, bsm1, bs2, bsm2, bsh;
    mp_ptr gp;
    enum toom7_flags aflags, bflags;
    TEMP_S_DECL;

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define a3 (numa + 3 * n)
#define a4 (numa + 4 * n)
#define a5 (numa + 5 * n)
#define b0 numb
#define b1 (numb + n)

    n = 1 + (na >= 3 * nb ? (na - 1) / (mp_size_t)6 : (nb - 1) >> 1);

    s = na - 5 * n;
    t = nb - n;

    lmmp_debug_assert(0 < s && s <= n);
    lmmp_debug_assert(0 < t && t <= n);
    
    mp_ptr restrict scratch = SALLOC_TYPE(10 * n + 10, mp_limb_t);

    mp_ptr restrict tmp = SALLOC_TYPE(10 * n + 10, mp_limb_t);
    as1 = tmp;
    asm1 = as1 + n + 1;
    as2 = asm1 + n + 1;
    asm2 = as2 + n + 1;
    ash = asm2 + n + 1;
    bs1 = ash + n + 1;
    bsm1 = bs1 + n + 1;
    bs2 = bsm1 + n;
    bsm2 = bs2 + n + 1;
    bsh = bsm2 + n + 1;

    gp = dst;

    /* Compute as1 and asm1.  */
    aflags = (enum toom7_flags)(toom7_w3_neg & lmmp_toom_eval_pm1_(as1, asm1, 5, numa, n, s, gp));

    /* Compute as2 and asm2. */
    aflags = (enum toom7_flags)(aflags | (toom7_w1_neg & lmmp_toom_eval_pm2_(as2, asm2, 5, numa, n, s, gp)));

    /* Compute ash = 32 a0 + 16 a1 + 8 a2 + 4 a3 + 2 a4 + a5
       = 2*(2*(2*(2*(2*a0 + a1) + a2) + a3) + a4) + a5  */

    cy = lmmp_addshl1_n_(ash, a1, a0, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a2, ash, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a3, ash, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a4, ash, n);
    if (s < n) {
        mp_limb_t cy2;
        cy2 = lmmp_addshl1_n_(ash, a5, ash, s);
        ash[n] = 2 * cy + lmmp_shl_(ash + s, ash + s, n - s, 1);
        lmmp_inc_1(ash + s, cy2);
    } else
        ash[n] = 2 * cy + lmmp_addshl1_n_(ash, a5, ash, n);

    /* Compute bs1 and bsm1.  */
    if (t == n) {
        if (lmmp_cmp_(b0, b1, n) < 0) {
            cy = lmmp_add_n_sub_n_(bs1, bsm1, b1, b0, n);
            bflags = toom7_w3_neg;
        } else {
            cy = lmmp_add_n_sub_n_(bs1, bsm1, b0, b1, n);
            bflags = (enum toom7_flags)0;
        }
        bs1[n] = cy >> 1;
    } else {
        bs1[n] = lmmp_add_(bs1, b0, n, b1, t);
        if (lmmp_zero_q_(b0 + t, n - t) && lmmp_cmp_(b0, b1, t) < 0) {
            lmmp_sub_n_(bsm1, b1, b0, t);
            lmmp_zero(bsm1 + t, n - t);
            bflags = toom7_w3_neg;
        } else {
            lmmp_sub_(bsm1, b0, n, b1, t);
            bflags = (enum toom7_flags)0;
        }
    }

    /* Compute bs2 and bsm2. Recycling bs1 and bsm1; bs2=bs1+b1, bsm2 =
       bsm1 - b1 */
    lmmp_add_(bs2, bs1, n + 1, b1, t);
    if (bflags & toom7_w3_neg) {
        bsm2[n] = lmmp_add_(bsm2, bsm1, n, b1, t);
        bflags = (enum toom7_flags)(bflags | toom7_w1_neg);
    } else {
        if (t < n) {
            if (lmmp_zero_q_(bsm1 + t, n - t) && lmmp_cmp_(bsm1, b1, t) < 0) {
                lmmp_sub_n_(bsm2, b1, bsm1, t);
                lmmp_zero(bsm2 + t, n + 1 - t);
                bflags = (enum toom7_flags)(bflags | toom7_w1_neg);
            } else {
                lmmp_sub_(bsm2, bsm1, n, b1, t);
                bsm2[n] = 0;
            }
        } else {
            if (lmmp_cmp_(bsm1, b1, n) < 0) {
                lmmp_sub_n_(bsm2, b1, bsm1, n);
                bflags = (enum toom7_flags)(bflags | toom7_w1_neg);
            } else {
                lmmp_sub_n_(bsm2, bsm1, b1, n);
            }
            bsm2[n] = 0;
        }
    }

    /* Compute bsh, recycling bs1. bsh=bs1+b0;  */
    bsh[n] = bs1[n] + lmmp_add_n_(bsh, bs1, b0, n);

    lmmp_debug_assert(as1[n] <= 5);
    lmmp_debug_assert(bs1[n] <= 1);
    lmmp_debug_assert(asm1[n] <= 2);
    lmmp_debug_assert(as2[n] <= 62);
    lmmp_debug_assert(bs2[n] <= 2);
    lmmp_debug_assert(asm2[n] <= 41);
    lmmp_debug_assert(bsm2[n] <= 1);
    lmmp_debug_assert(ash[n] <= 62);
    lmmp_debug_assert(bsh[n] <= 2);

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
    lmmp_mul_n_(vm1, asm1, bsm1, n);
    cy = 0;
    if (asm1[n] == 1) {
        cy = lmmp_add_n_(vm1 + n, vm1 + n, bsm1, n);
    } else if (asm1[n] == 2) {
        cy = lmmp_addshl1_n_(vm1 + n, vm1 + n, bsm1, n);
    }
    vm1[2 * n] = cy;

    /* v1, 2n+1 limbs */
    lmmp_mul_n_(v1, as1, bs1, n);
    if (as1[n] == 1) {
        cy = bs1[n] + lmmp_add_n_(v1 + n, v1 + n, bs1, n);
    } else if (as1[n] == 2) {
        cy = 2 * bs1[n] + lmmp_addshl1_n_(v1 + n, v1 + n, bs1, n);
    } else if (as1[n] != 0) {
        cy = as1[n] * bs1[n] + lmmp_addmul_1_(v1 + n, bs1, n, as1[n]);
    } else
        cy = 0;
    if (bs1[n] != 0)
        cy += lmmp_add_n_(v1 + n, v1 + n, as1, n);
    v1[2 * n] = cy;

    lmmp_mul_n_(v0, a0, b0, n); /* v0, 2n limbs */

    /* vinf, s+t limbs */
    if (s > t)
        lmmp_mul_(vinf, a5, s, b1, t);
    else
        lmmp_mul_(vinf, b1, t, a5, s);

    lmmp_toom_interp7_(dst, n, (enum toom7_flags)(aflags ^ bflags), vm2, vm1, v2, vh, s + t, scratch_out);

    TEMP_S_FREE;
}

typedef struct {
    mp_srcptr restrict numb;
    mp_size_t n;
    mp_size_t s;
    mp_size_t t;
    mp_ptr restrict scratch;
    mp_ptr restrict tmp;
    mp_ptr restrict bs1;
    mp_ptr restrict bsm1;
    mp_ptr restrict bs2;
    mp_ptr restrict bsm2;
    mp_ptr restrict bsh;
} toom62_cache_t;

static enum toom7_flags lmmp_mul_toom62_cache_init_(
    mp_ptr    restrict   dst,
    mp_srcptr restrict  numa,
    toom62_cache_t*    cache
) {
#define numb (cache->numb)
#define n (cache->n)
#define s (cache->s)
#define t (cache->t)
#define scratch (cache->scratch)
#define tmp (cache->tmp)
#define bs1 (cache->bs1)
#define bsm1 (cache->bsm1)
#define bs2 (cache->bs2)
#define bsm2 (cache->bsm2)
#define bsh (cache->bsh)

    mp_limb_t cy;
    mp_ptr restrict as1, asm1, as2, asm2, ash;
    enum toom7_flags aflags, bflags;

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define a3 (numa + 3 * n)
#define a4 (numa + 4 * n)
#define a5 (numa + 5 * n)
#define b0 numb
#define b1 (numb + n)


    as1 = tmp;
    asm1 = as1 + n + 1;
    as2 = asm1 + n + 1;
    asm2 = as2 + n + 1;
    ash = asm2 + n + 1;


    /* Compute as1 and asm1.  */
    aflags = (enum toom7_flags)(toom7_w3_neg & lmmp_toom_eval_pm1_(as1, asm1, 5, numa, n, s, dst));

    /* Compute as2 and asm2. */
    aflags = (enum toom7_flags)(aflags | (toom7_w1_neg & lmmp_toom_eval_pm2_(as2, asm2, 5, numa, n, s, dst)));

    /* Compute ash = 32 a0 + 16 a1 + 8 a2 + 4 a3 + 2 a4 + a5
       = 2*(2*(2*(2*(2*a0 + a1) + a2) + a3) + a4) + a5  */

    cy = lmmp_addshl1_n_(ash, a1, a0, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a2, ash, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a3, ash, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a4, ash, n);
    if (s < n) {
        mp_limb_t cy2;
        cy2 = lmmp_addshl1_n_(ash, a5, ash, s);
        ash[n] = 2 * cy + lmmp_shl_(ash + s, ash + s, n - s, 1);
        lmmp_inc_1(ash + s, cy2);
    } else
        ash[n] = 2 * cy + lmmp_addshl1_n_(ash, a5, ash, n);

    /* Compute bs1 and bsm1.  */
    if (t == n) {
        if (lmmp_cmp_(b0, b1, n) < 0) {
            cy = lmmp_add_n_sub_n_(bs1, bsm1, b1, b0, n);
            bflags = toom7_w3_neg;
        } else {
            cy = lmmp_add_n_sub_n_(bs1, bsm1, b0, b1, n);
            bflags = (enum toom7_flags)0;
        }
        bs1[n] = cy >> 1;
    } else {
        bs1[n] = lmmp_add_(bs1, b0, n, b1, t);
        if (lmmp_zero_q_(b0 + t, n - t) && lmmp_cmp_(b0, b1, t) < 0) {
            lmmp_sub_n_(bsm1, b1, b0, t);
            lmmp_zero(bsm1 + t, n - t);
            bflags = toom7_w3_neg;
        } else {
            lmmp_sub_(bsm1, b0, n, b1, t);
            bflags = (enum toom7_flags)0;
        }
    }

    /* Compute bs2 and bsm2. Recycling bs1 and bsm1; bs2=bs1+b1, bsm2 =
       bsm1 - b1 */
    lmmp_add_(bs2, bs1, n + 1, b1, t);
    if (bflags & toom7_w3_neg) {
        bsm2[n] = lmmp_add_(bsm2, bsm1, n, b1, t);
        bflags = (enum toom7_flags)(bflags | toom7_w1_neg);
    } else {
        if (t < n) {
            if (lmmp_zero_q_(bsm1 + t, n - t) && lmmp_cmp_(bsm1, b1, t) < 0) {
                lmmp_sub_n_(bsm2, b1, bsm1, t);
                lmmp_zero(bsm2 + t, n + 1 - t);
                bflags = (enum toom7_flags)(bflags | toom7_w1_neg);
            } else {
                lmmp_sub_(bsm2, bsm1, n, b1, t);
                bsm2[n] = 0;
            }
        } else {
            if (lmmp_cmp_(bsm1, b1, n) < 0) {
                lmmp_sub_n_(bsm2, b1, bsm1, n);
                bflags = (enum toom7_flags)(bflags | toom7_w1_neg);
            } else {
                lmmp_sub_n_(bsm2, bsm1, b1, n);
            }
            bsm2[n] = 0;
        }
    }

    /* Compute bsh, recycling bs1. bsh=bs1+b0;  */
    bsh[n] = bs1[n] + lmmp_add_n_(bsh, bs1, b0, n);

    lmmp_debug_assert(as1[n] <= 5);
    lmmp_debug_assert(bs1[n] <= 1);
    lmmp_debug_assert(asm1[n] <= 2);
    lmmp_debug_assert(as2[n] <= 62);
    lmmp_debug_assert(bs2[n] <= 2);
    lmmp_debug_assert(asm2[n] <= 41);
    lmmp_debug_assert(bsm2[n] <= 1);
    lmmp_debug_assert(ash[n] <= 62);
    lmmp_debug_assert(bsh[n] <= 2);

#define v0 dst                            /* 2n   */
#define v1 (dst + 2 * n)                  /* 2n+1 */
#define vinf (dst + 6 * n)                /* s+t  */
#define v2 scratch                        /* 2n+1 */
#define vm2 (scratch + 2 * n + 1)         /* 2n+1 */
#define vh (scratch + 4 * n + 2)          /* 2n+1 */
#define vm1 (scratch + 6 * n + 3)         /* 2n+1 */
#define scratch_out (scratch + 8 * n + 4) /* 2n+1 */
    /* Total scratch need: 10*n+5 */

    /* Must be in allocation order, as they overwrite one limb beyond
     * 2n+1. */
    lmmp_mul_n_(v2, as2, bs2, n + 1);    /* v2, 2n+1 limbs */
    lmmp_mul_n_(vm2, asm2, bsm2, n + 1); /* vm2, 2n+1 limbs */
    lmmp_mul_n_(vh, ash, bsh, n + 1);    /* vh, 2n+1 limbs */

    /* vm1, 2n+1 limbs */
    lmmp_mul_n_(vm1, asm1, bsm1, n);
    cy = 0;
    if (asm1[n] == 1) {
        cy = lmmp_add_n_(vm1 + n, vm1 + n, bsm1, n);
    } else if (asm1[n] == 2) {
        cy = lmmp_addshl1_n_(vm1 + n, vm1 + n, bsm1, n);
    }
    vm1[2 * n] = cy;

    /* v1, 2n+1 limbs */
    lmmp_mul_n_(v1, as1, bs1, n);
    if (as1[n] == 1) {
        cy = bs1[n] + lmmp_add_n_(v1 + n, v1 + n, bs1, n);
    } else if (as1[n] == 2) {
        cy = 2 * bs1[n] + lmmp_addshl1_n_(v1 + n, v1 + n, bs1, n);
    } else if (as1[n] != 0) {
        cy = as1[n] * bs1[n] + lmmp_addmul_1_(v1 + n, bs1, n, as1[n]);
    } else
        cy = 0;
    if (bs1[n] != 0)
        cy += lmmp_add_n_(v1 + n, v1 + n, as1, n);
    v1[2 * n] = cy;

    lmmp_mul_n_(v0, a0, b0, n); /* v0, 2n limbs */

    /* vinf, s+t limbs */
    if (s > t)
        lmmp_mul_(vinf, a5, s, b1, t);
    else
        lmmp_mul_(vinf, b1, t, a5, s);

    lmmp_toom_interp7_(dst, n, (enum toom7_flags)(aflags ^ bflags), vm2, vm1, v2, vh, s + t, scratch_out);

    return bflags;
#undef numb
#undef n
#undef s
#undef t
#undef scratch
#undef tmp
#undef bs1
#undef bsm1
#undef bs2
#undef bsm2
#undef bsh
}

static void lmmp_mul_toom62_cache_(
    mp_ptr    restrict      dst,
    mp_srcptr restrict     numa,
    const toom62_cache_t* cache,
    enum toom7_flags     bflags
) {
#define numb (cache->numb)
#define n (cache->n)
#define s (cache->s)
#define t (cache->t)
#define scratch (cache->scratch)
#define tmp (cache->tmp)
#define bs1 (cache->bs1)
#define bsm1 (cache->bsm1)
#define bs2 (cache->bs2)
#define bsm2 (cache->bsm2)
#define bsh (cache->bsh)

    mp_limb_t cy;
    mp_ptr as1, asm1, as2, asm2, ash;
    enum toom7_flags aflags;

#define a0 numa
#define a1 (numa + n)
#define a2 (numa + 2 * n)
#define a3 (numa + 3 * n)
#define a4 (numa + 4 * n)
#define a5 (numa + 5 * n)
#define b0 numb
#define b1 (numb + n)

    as1 = tmp;
    asm1 = as1 + n + 1;
    as2 = asm1 + n + 1;
    asm2 = as2 + n + 1;
    ash = asm2 + n + 1;


    /* Compute as1 and asm1.  */
    aflags = (enum toom7_flags)(toom7_w3_neg & lmmp_toom_eval_pm1_(as1, asm1, 5, numa, n, s, dst));

    /* Compute as2 and asm2. */
    aflags = (enum toom7_flags)(aflags | (toom7_w1_neg & lmmp_toom_eval_pm2_(as2, asm2, 5, numa, n, s, dst)));

    /* Compute ash = 32 a0 + 16 a1 + 8 a2 + 4 a3 + 2 a4 + a5
       = 2*(2*(2*(2*(2*a0 + a1) + a2) + a3) + a4) + a5  */

    cy = lmmp_addshl1_n_(ash, a1, a0, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a2, ash, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a3, ash, n);
    cy = 2 * cy + lmmp_addshl1_n_(ash, a4, ash, n);
    if (s < n) {
        mp_limb_t cy2;
        cy2 = lmmp_addshl1_n_(ash, a5, ash, s);
        ash[n] = 2 * cy + lmmp_shl_(ash + s, ash + s, n - s, 1);
        lmmp_inc_1(ash + s, cy2);
    } else
        ash[n] = 2 * cy + lmmp_addshl1_n_(ash, a5, ash, n);

    lmmp_debug_assert(as1[n] <= 5);
    lmmp_debug_assert(asm1[n] <= 2);
    lmmp_debug_assert(as2[n] <= 62);
    lmmp_debug_assert(asm2[n] <= 41);
    lmmp_debug_assert(ash[n] <= 62);

#define v0 dst                            /* 2n   */
#define v1 (dst + 2 * n)                  /* 2n+1 */
#define vinf (dst + 6 * n)                /* s+t  */
#define v2 scratch                        /* 2n+1 */
#define vm2 (scratch + 2 * n + 1)         /* 2n+1 */
#define vh (scratch + 4 * n + 2)          /* 2n+1 */
#define vm1 (scratch + 6 * n + 3)         /* 2n+1 */
#define scratch_out (scratch + 8 * n + 4) /* 2n+1 */
    /* Total scratch need: 10*n+5 */

    /* Must be in allocation order, as they overwrite one limb beyond
     * 2n+1. */
    lmmp_mul_n_(v2, as2, bs2, n + 1);    /* v2, 2n+1 limbs */
    lmmp_mul_n_(vm2, asm2, bsm2, n + 1); /* vm2, 2n+1 limbs */
    lmmp_mul_n_(vh, ash, bsh, n + 1);    /* vh, 2n+1 limbs */

    /* vm1, 2n+1 limbs */
    lmmp_mul_n_(vm1, asm1, bsm1, n);
    cy = 0;
    if (asm1[n] == 1) {
        cy = lmmp_add_n_(vm1 + n, vm1 + n, bsm1, n);
    } else if (asm1[n] == 2) {
        cy = lmmp_addshl1_n_(vm1 + n, vm1 + n, bsm1, n);
    }
    vm1[2 * n] = cy;

    /* v1, 2n+1 limbs */
    lmmp_mul_n_(v1, as1, bs1, n);
    if (as1[n] == 1) {
        cy = bs1[n] + lmmp_add_n_(v1 + n, v1 + n, bs1, n);
    } else if (as1[n] == 2) {
        cy = 2 * bs1[n] + lmmp_addshl1_n_(v1 + n, v1 + n, bs1, n);
    } else if (as1[n] != 0) {
        cy = as1[n] * bs1[n] + lmmp_addmul_1_(v1 + n, bs1, n, as1[n]);
    } else
        cy = 0;
    if (bs1[n] != 0)
        cy += lmmp_add_n_(v1 + n, v1 + n, as1, n);
    v1[2 * n] = cy;

    lmmp_mul_n_(v0, a0, b0, n); /* v0, 2n limbs */

    /* vinf, s+t limbs */
    if (s > t)
        lmmp_mul_(vinf, a5, s, b1, t);
    else
        lmmp_mul_(vinf, b1, t, a5, s);

    lmmp_toom_interp7_(dst, n, (enum toom7_flags)(aflags ^ bflags), vm2, vm1, v2, vh, s + t, scratch_out);

#undef numb
#undef n
#undef s
#undef t
#undef scratch
#undef tmp
#undef bs1
#undef bsm1
#undef bs2
#undef bsm2
#undef bsh
}

void lmmp_mul_toom62_unbalance_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_size_t            na,
    mp_srcptr restrict numb,
    mp_size_t            nb
) {
    lmmp_param_assert(na >= 5 * nb);
    TEMP_DECL;
    mp_limb_t* restrict ws = SALLOC_TYPE(nb, mp_limb_t);

    toom62_cache_t cache;
    cache.numb = numb;
    cache.n = 1 + (3 * nb - 1) / (mp_size_t)6;
    cache.s = 3 * nb - 5 * cache.n;
    cache.t = nb - cache.n;
    cache.scratch = BALLOC_TYPE(20 * cache.n + 20, mp_limb_t);
    cache.tmp = cache.scratch + 10 * cache.n + 10;
    cache.bs1 = cache.tmp + 5 * cache.n + 5;
    cache.bsm1 = cache.bs1 + cache.n + 1;
    cache.bs2 = cache.bsm1 + cache.n;
    cache.bsm2 = cache.bs2 + cache.n + 1;
    cache.bsh = cache.bsm2 + cache.n + 1;
    
    enum toom7_flags bflags = lmmp_mul_toom62_cache_init_(dst, numa, &cache);
    dst += 3 * nb;
    numa += 3 * nb;
    na -= 3 * nb;
    lmmp_copy(ws, dst, nb);
    while (na >= 5 * nb) {
        lmmp_mul_toom62_cache_(dst, numa, &cache, bflags);
        if (lmmp_add_n_(dst, dst, ws, nb))
            lmmp_inc(dst + nb);
        dst += 3 * nb;
        numa += 3 * nb;
        na -= 3 * nb;
        lmmp_copy(ws, dst, nb);
    }
    // 0 <= na < 2 nb
    if (na >= nb)
        lmmp_mul_(dst, numa, na, numb, nb);
    else
        lmmp_mul_(dst, numb, nb, numa, na);
    if (lmmp_add_n_(dst, dst, ws, nb))
        lmmp_inc(dst + nb);
    TEMP_FREE;
}

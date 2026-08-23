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
#define lmmp_mul_n_(dst, numa, numb, n)                      \
    if ((n) < MUL_TOOM22_THRESHOLD)                          \
        lmmp_mul_basecase_((dst), (numa), (n), (numb), (n)); \
    else                                                     \
        lmmp_mul_toom22_((dst), (numa), (n), (numb), (n))
#endif

/*
Evaluate in: -1, 0, +inf

   <-s--><--n-->
   |-a1-|--a0--|
    |b1-|--b0--|
    <-t-><--n-->

v0  =  a0    * b0      #   A(0)*B(0)
vm1 = (a0-a1)*(b0-b1)  #  A(-1)*B(-1)
vinf=     a1 *    b1   # A(inf)*B(inf)
*/

void lmmp_mul_toom22_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(nb >= 5);
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(4 * na <= 5 * nb);
    TEMP_S_DECL;
    mp_size_t s = na >> 1, n = na - s, t = nb - n;
    mp_limb_t* restrict vm1 = SALLOC_TYPE(2 * n, mp_limb_t);
    int vm1_neg = 0;
    mp_slimb_t cy, cy2;

#define a0 numa
#define a1 (numa + n)
#define b0 numb
#define b1 (numb + n)
#define asm1 dst
#define bsm1 (dst + n)

    if (s == n) {
        if (lmmp_cmp_(a0, a1, n) < 0) {
            lmmp_sub_n_(asm1, a1, a0, n);
            vm1_neg = 1;
        } else
            lmmp_sub_n_(asm1, a0, a1, n);
    } else {  // s==n-1
        if (a0[s] == 0 && lmmp_cmp_(a0, a1, s) < 0) {
            lmmp_sub_n_(asm1, a1, a0, s);
            asm1[s] = 0;
            vm1_neg = 1;
        } else
            asm1[s] = a0[s] - lmmp_sub_n_(asm1, a0, a1, s);
    }

    if (t == n) {
        if (lmmp_cmp_(b0, b1, n) < 0) {
            lmmp_sub_n_(bsm1, b1, b0, n);
            vm1_neg ^= 1;
        } else
            lmmp_sub_n_(bsm1, b0, b1, n);
    } else {
        if (lmmp_zero_q_(b0 + t, n - t) && lmmp_cmp_(b0, b1, t) < 0) {
            lmmp_sub_n_(bsm1, b1, b0, t);
            lmmp_zero(bsm1 + t, n - t);
            vm1_neg ^= 1;
        } else
            lmmp_sub_(bsm1, b0, n, b1, t);
    }

    lmmp_mul_n_(vm1, asm1, bsm1, n);

#undef asm1
#undef bsm1
#define v0 dst
#define vinf (dst + 2 * n)

    lmmp_mul_n_(v0, a0, b0, n);

    lmmp_mul_(vinf, a1, s, b1, t);

    cy = lmmp_add_n_(dst + 2 * n, v0 + n, vinf, n);
    cy2 = cy + lmmp_add_n_(dst + n, dst + 2 * n, v0, n);
    cy += lmmp_add_(dst + 2 * n, dst + 2 * n, n, vinf + n, s + t - n);

    if (vm1_neg)
        cy += lmmp_add_n_(dst + n, dst + n, vm1, 2 * n);
    else
        cy -= lmmp_sub_n_(dst + n, dst + n, vm1, 2 * n);

    // no overflow, if s+t>n. proved.
    lmmp_inc_1(dst + 2 * n, cy2);

    if (cy < 0)
        lmmp_dec(dst + 3 * n);
    else
        lmmp_inc_1(dst + 3 * n, cy);
    TEMP_S_FREE;
}

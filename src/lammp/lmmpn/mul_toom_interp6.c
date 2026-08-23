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

#include "../../../include/lammp/impl/divexact.h"
#include "../../../include/lammp/impl/toom_interp.h"


/*
    syms x integer
    a = 2*x^3 + 32*x^2 + 4*x + 5;
    b = 6*x^2 + 712*x + 18;
    c_true = a*b;

    w0 = 2*6;
    w1 = subs(a,2)*subs(b,2);
    w2 = subs(a,-2)*subs(b,-2);
    w3 = subs(a,1)*subs(b,1);
    w4 = subs(a,-1)*subs(b,-1);
    w5 = subs(a,0)*subs(b,0);

    w2 = (w1 - w2)/4;
    w1 = (w1 - w5)/2;
    w1 = (w1 - w2)/2;
    w4 = (w3 - w4)/2;
    w2 = (w2 - w4)/3;
    w3 = w3 - w4 - w5;
    w1 = (w1 - w3)/3;
    w2 = w2 - w0*4;
    w4 = w4 - w2;
    w3 = w3 - w1;
    w2 = w2 - w0;

    c0=w5; c1=w4; c2=w3; c3=w2; c4=w1; c5=w0;
    c_calc = c5*x^5 + c4*x^4 + c3*x^3 + c2*x^2 + c1*x + c0;
*/

void lmmp_toom_interp6_(
              mp_ptr   dst,
           mp_size_t     n,
    enum toom6_flags flags,
              mp_ptr    w4,
              mp_ptr    w2,
              mp_ptr    w1,
           mp_size_t   w0n
) {

    lmmp_param_assert(n > 0);
    lmmp_param_assert(2 * n >= w0n && w0n > 0);
    mp_limb_t cy;
    /* cy6 can be stored in w1[2*n], cy4 in w4[0], embankment in w2[0] */
    mp_limb_t cy4, cy6, embankment;

#define w5 dst           /* 2n   */
#define w3 (dst + 2 * n) /* 2n+1 */
#define w0 (dst + 5 * n) /* w0n  */

    /* W2 =(W1 - W2)>>2 */
    if (flags & toom6_vm2_neg)
        lmmp_add_n_(w2, w1, w2, 2 * n + 1);
    else
        lmmp_sub_n_(w2, w1, w2, 2 * n + 1);
    lmmp_shr_(w2, w2, 2 * n + 1, 2);

    /* W1 =(W1 - W5)>>1 */
    w1[2 * n] -= lmmp_sub_n_(w1, w1, w5, 2 * n);
    lmmp_shr_(w1, w1, 2 * n + 1, 1);

    /* W1 =(W1 - W2)>>1 */
    lmmp_shr1sub_n_(w1, w1, w2, 2 * n + 1);

    /* W4 =(W3 - W4)>>1 */
    if (flags & toom6_vm1_neg) {
        lmmp_shr1add_n_(w4, w3, w4, 2 * n + 1);
    } else {
        lmmp_shr1sub_n_(w4, w3, w4, 2 * n + 1);
    }

    /* W2 =(W2 - W4)/3 */
    lmmp_sub_n_(w2, w2, w4, 2 * n + 1);
    lmmp_divexact_by3_(w2, w2, 2 * n + 1);

    /* W3 = W3 - W4 - W5 */
    lmmp_sub_n_(w3, w3, w4, 2 * n + 1);
    w3[2 * n] -= lmmp_sub_n_(w3, w3, w5, 2 * n);

    /* W1 =(W1 - W3)/3 */
    lmmp_sub_n_(w1, w1, w3, 2 * n + 1);
    lmmp_divexact_by3_(w1, w1, 2 * n + 1);

    cy = lmmp_add_n_(dst + n, dst + n, w4, 2 * n + 1);
    lmmp_inc_1(dst + 3 * n + 1, cy);

    /* W2 -= W0<<2 */
    /* {W4,2*n+1} is now free and can be overwritten. */
    cy = lmmp_shl_(w4, w0, w0n, 2);
    cy += lmmp_sub_n_(w2, w2, w4, w0n);

    lmmp_dec_1(w2 + w0n, cy);

    /* W4L = W4L - W2L */
    cy = lmmp_sub_n_(dst + n, dst + n, w2, n);
    lmmp_dec_1(w3, cy);

    /* W3H = W3H + W2L */
    cy4 = w3[2 * n] + lmmp_add_n_(dst + 3 * n, dst + 3 * n, w2, n);
    /* W1L + W2H */
    cy = w2[2 * n] + lmmp_add_n_(dst + 4 * n, w1, w2 + n, n);
    lmmp_inc_1(w1 + n, cy);

    /* W0 = W0 + W1H */
    if (w0n > n)
        cy6 = w1[2 * n] + lmmp_add_n_(w0, w0, w1 + n, n);
    else
        cy6 = lmmp_add_n_(w0, w0, w1 + n, w0n);

    cy = lmmp_sub_n_(dst + 2 * n, dst + 2 * n, dst + 4 * n, n + w0n);

    /* embankment is a "dirty trick" to avoid carry/borrow propagation
       beyond allocated memory */
    embankment = w0[w0n - 1] - 1;
    w0[w0n - 1] = 1;
    if (w0n > n) {
        if (cy4 > cy6)
            lmmp_inc_1(dst + 4 * n, cy4 - cy6);
        else
            lmmp_dec_1(dst + 4 * n, cy6 - cy4);
        lmmp_dec_1(dst + 3 * n + w0n, cy);
        lmmp_inc_1(w0 + n, cy6);
    } else {
        lmmp_inc_1(dst + 4 * n, cy4);
        lmmp_dec_1(dst + 3 * n + w0n, cy + cy6);
    }
    w0[w0n - 1] += embankment;

#undef w5
#undef w3
#undef w0
}

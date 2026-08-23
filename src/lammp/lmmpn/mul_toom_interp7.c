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
    a = 12*x^3 + 22*x^2 + 32*x + 42;
    b = 56*x^3 + 62*x^2 + 7*x + 82;
    c_true = a*b;

    w0 = subs(a,0)*subs(b,0);
    w1 = subs(a,-2)*subs(b,-2);
    w2 = subs(a,1)*subs(b,1);
    w3 = subs(a,-1)*subs(b,-1);
    w4 = subs(a,2)*subs(b,2);
    w5 = 64*subs(a,1/2)*subs(b,1/2);
    w6 = 12*56;

    w5 = w5 + w4;
    w1 = (w4 - w1)/2;
    w4 = w4 - w0;
    w4 = (w4 - w1)/4 - w6*16;
    w3 = (w2 - w3)/2;
    w2 = w2 - w3;
    w5 = w5 - 65*w2;
    w2 = w2 - w6 - w0;
    w5 = (w5 + 45*w2)/2;
    w4 = (w4 - w2)/3;
    w2 = w2 - w4;
    w1 = w5 - w1;
    w5 = (w5 - 8*w3)/9;
    w3 = w3 - w5;
    w1 = (w1/15 + w5)/2;
    w5 = w5 - w1;

    c0 = w0; c1 = w1; c2 = w2; c3 = w3; c4 = w4; c5 = w5; c6 = w6;
    c_calc = c6*x^6 + c5*x^5 + c4*x^4 + c3*x^3 + c2*x^2 + c1*x + c0;
*/

void lmmp_toom_interp7_(
              mp_ptr   dst,
           mp_size_t     n,
    enum toom7_flags flags,
              mp_ptr    w1,
              mp_ptr    w3,
              mp_ptr    w4,
              mp_ptr    w5,
           mp_size_t   w6n,
              mp_ptr    tp
) {
    lmmp_param_assert(w6n > 0);
    lmmp_param_assert(w6n <= 2 * n);
    mp_size_t m;
    mp_limb_t cy;

    m = 2 * n + 1;
#define w0 dst
#define w2 (dst + 2 * n)
#define w6 (dst + 6 * n)

    lmmp_add_n_(w5, w5, w4, m);
    if (flags & toom7_w1_neg) {
        lmmp_shr1add_n_(w1, w1, w4, m);
    } else {
        lmmp_shr1sub_n_(w1, w4, w1, m);
    }
    lmmp_sub_(w4, w4, m, w0, 2 * n);
    lmmp_sub_n_(w4, w4, w1, m);

    lmmp_debug_assert(!(w4[0] & 3));

    lmmp_shr_(w4, w4, m, 2); /* w4>=0 */

    tp[w6n] = lmmp_shl_(tp, w6, w6n, 4);
    lmmp_sub_(w4, w4, m, tp, w6n + 1);

    if (flags & toom7_w3_neg) {
        lmmp_shr1add_n_(w3, w3, w2, m);
    } else {
        lmmp_shr1sub_n_(w3, w2, w3, m);
    }

    lmmp_sub_n_(w2, w2, w3, m);

    lmmp_submul_1_(w5, w2, m, 65);
    lmmp_sub_(w2, w2, m, w6, w6n);
    lmmp_sub_(w2, w2, m, w0, 2 * n);

    lmmp_addmul_1_(w5, w2, m, 45);
    lmmp_debug_assert(!(w5[0] & 1));
    lmmp_shr_(w5, w5, m, 1);
    lmmp_sub_n_(w4, w4, w2, m);

    lmmp_divexact_by3_(w4, w4, m);
    lmmp_sub_n_(w2, w2, w4, m);

    lmmp_sub_n_(w1, w5, w1, m);
    lmmp_shl_(tp, w3, m, 3);
    lmmp_sub_n_(w5, w5, tp, m);
    lmmp_divexact_by9_(w5, w5, m);
    lmmp_sub_n_(w3, w3, w5, m);

    lmmp_divexact_by15_(w1, w1, m);
    lmmp_shr1add_n_(w1, w1, w5, m);
    w1[m - 1] &= LIMB_MAX >> 1;

    lmmp_sub_n_(w5, w5, w1, m);

    /* These bounds are valid for the 4x4 polynomial product of toom44,
     * and they are conservative for toom53 and toom62. */
    lmmp_debug_assert(w1[2 * n] < 2);
    lmmp_debug_assert(w2[2 * n] < 3);
    lmmp_debug_assert(w3[2 * n] < 4);
    lmmp_debug_assert(w4[2 * n] < 3);
    lmmp_debug_assert(w5[2 * n] < 2);

    cy = lmmp_add_n_(dst + n, dst + n, w1, m);
    lmmp_inc_1(w2 + n + 1, cy);
    cy = lmmp_add_n_(dst + 3 * n, dst + 3 * n, w3, n);
    lmmp_inc_1(w3 + n, w2[2 * n] + cy);
    cy = lmmp_add_n_(dst + 4 * n, w3 + n, w4, n);
    lmmp_inc_1(w4 + n, w3[2 * n] + cy);
    cy = lmmp_add_n_(dst + 5 * n, w4 + n, w5, n);
    lmmp_inc_1(w5 + n, w4[2 * n] + cy);
    if (w6n > n + 1) {
        cy = lmmp_add_n_(dst + 6 * n, dst + 6 * n, w5 + n, n + 1);
        lmmp_inc_1(dst + 7 * n + 1, cy);
    } else {
        cy = lmmp_add_n_(dst + 6 * n, dst + 6 * n, w5 + n, w6n);
        lmmp_debug_assert(cy == 0);
    }
}

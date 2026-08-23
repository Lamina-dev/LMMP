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

#include "../../../include/lammp/impl/toom_interp.h"


int lmmp_toom_eval_dgr3_pm1_(mp_ptr xp1, mp_ptr xm1, mp_srcptr xp, mp_size_t n, mp_size_t x3n, mp_ptr tp) {
    int neg;
    lmmp_param_assert(x3n > 0);
    lmmp_param_assert(x3n <= n);

    xp1[n] = lmmp_add_n_(xp1, xp, xp + 2 * n, n);
    tp[n] = lmmp_add_(tp, xp + n, n, xp + 3 * n, x3n);

    neg = (lmmp_cmp_(xp1, tp, n + 1) < 0) ? ~0 : 0;
    if (neg)
        lmmp_add_n_sub_n_(xp1, xm1, tp, xp1, n + 1);
    else
        lmmp_add_n_sub_n_(xp1, xm1, xp1, tp, n + 1);

    lmmp_debug_assert(xp1[n] <= 3);
    lmmp_debug_assert(xm1[n] <= 1);

    return neg;
}

int lmmp_toom_eval_dgr3_pm2_(mp_ptr xp2, mp_ptr xm2, mp_srcptr xp, mp_size_t n, mp_size_t x3n, mp_ptr tp) {
    mp_limb_t cy;
    int neg;
    lmmp_param_assert(x3n > 0);
    lmmp_param_assert(x3n <= n);
    /* (x0 + 4 * x2) +/- (2 x1 + 8 x_3) */

    cy = lmmp_shl_(tp, xp + 2 * n, n, 2);
    xp2[n] = cy + lmmp_add_n_(xp2, tp, xp, n);

    tp[x3n] = lmmp_shl_(tp, xp + 3 * n, x3n, 2);
    if (x3n < n)
        tp[n] = lmmp_add_(tp, xp + n, n, tp, x3n + 1);
    else
        tp[n] += lmmp_add_n_(tp, xp + n, tp, n);

    lmmp_shl_(tp, tp, n + 1, 1);

    neg = (lmmp_cmp_(xp2, tp, n + 1) < 0) ? ~0 : 0;

    if (neg)
        lmmp_add_n_sub_n_(xp2, xm2, tp, xp2, n + 1);
    else
        lmmp_add_n_sub_n_(xp2, xm2, xp2, tp, n + 1);

    lmmp_debug_assert(xp2[n] < 15);
    lmmp_debug_assert(xm2[n] < 10);

    return neg;
}

int lmmp_toom_eval_pm1_(mp_ptr xp1, mp_ptr xm1, unsigned k, mp_srcptr xp, mp_size_t n, mp_size_t hn, mp_ptr tp) {
    unsigned i;
    int neg;
    lmmp_param_assert(k >= 4);

    lmmp_param_assert(hn > 0);
    lmmp_param_assert(hn <= n);

    /* The degree k is also the number of full-size coefficients, so
     * that last coefficient, of size hn, starts at xp + k*n. */

    xp1[n] = lmmp_add_n_(xp1, xp, xp + 2 * n, n);
    for (i = 4; i < k; i += 2) lmmp_add_(xp1, xp1, n + 1, xp + i * n, n);

    tp[n] = lmmp_add_n_(tp, xp + n, xp + 3 * n, n);
    for (i = 5; i < k; i += 2) lmmp_add_(tp, tp, n + 1, xp + i * n, n);

    if (k & 1)
        lmmp_add_(tp, tp, n + 1, xp + k * n, hn);
    else
        lmmp_add_(xp1, xp1, n + 1, xp + k * n, hn);

    neg = (lmmp_cmp_(xp1, tp, n + 1) < 0) ? ~0 : 0;

    if (neg)
        lmmp_add_n_sub_n_(xp1, xm1, tp, xp1, n + 1);
    else
        lmmp_add_n_sub_n_(xp1, xm1, xp1, tp, n + 1);

    lmmp_debug_assert(xp1[n] <= k);
    lmmp_debug_assert(xm1[n] <= k / 2 + 1);

    return neg;
}

/* DO_addlsh2(d,a,b,n,cy) computes cy,[d,n] <= [a,n] + 4*(cy,[b,n]), it
   can be used as DO_addlsh2(d,a,d,n,d[n]), for accumulation on [d,n+1]. */

/* The following is not a general substitute for addlsh2.
   It is correct if d == b, but it is not if d == a.  */
#define DO_addlsh2(d, a, b, n, cy)       \
    do {                                 \
        (cy) <<= 2;                      \
        (cy) += lmmp_shl_(d, b, n, 2);   \
        (cy) += lmmp_add_n_(d, d, a, n); \
    } while (0)

int lmmp_toom_eval_pm2_(mp_ptr xp2, mp_ptr xm2, unsigned k, mp_srcptr xp, mp_size_t n, mp_size_t hn, mp_ptr tp) {
    int i;
    int neg;
    mp_limb_t cy;
    lmmp_param_assert(k >= 3);
    lmmp_param_assert(k < LIMB_BITS);

    lmmp_param_assert(hn > 0);
    lmmp_param_assert(hn <= n);

    /* The degree k is also the number of full-size coefficients, so
     * that last coefficient, of size hn, starts at xp + k*n. */

    cy = 0;
    DO_addlsh2(xp2, xp + (k - 2) * n, xp + k * n, hn, cy);
    if (hn != n)
        cy = lmmp_add_1_(xp2 + hn, xp + (k - 2) * n + hn, n - hn, cy);
    for (i = k - 4; i >= 0; i -= 2) DO_addlsh2(xp2, xp + i * n, xp2, n, cy);
    xp2[n] = cy;

    k--;

    cy = 0;
    DO_addlsh2(tp, xp + (k - 2) * n, xp + k * n, n, cy);
    for (i = k - 4; i >= 0; i -= 2) DO_addlsh2(tp, xp + i * n, tp, n, cy);
    tp[n] = cy;

    if (k & 1)
        lmmp_shl_(tp, tp, n + 1, 1);
    else
        lmmp_shl_(xp2, xp2, n + 1, 1);

    neg = (lmmp_cmp_(xp2, tp, n + 1) < 0) ? ~0 : 0;

    if (neg)
        lmmp_add_n_sub_n_(xp2, xm2, tp, xp2, n + 1);
    else
        lmmp_add_n_sub_n_(xp2, xm2, xp2, tp, n + 1);

    lmmp_debug_assert(xp2[n] < (1ull << (k + 2)) - 1);
    lmmp_debug_assert(xm2[n] < ((1 << (k + 3)) - 1 - (1 ^ (k & 1))) / 3);

    neg ^= ((k & 1) - 1);

    return neg;
}

#undef DO_addlsh2

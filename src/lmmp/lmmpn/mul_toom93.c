/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "impl/mul_hard.h"
#include "mparam.h"
#include "lmmpn/toom_interp.h"

LMMP_VECT_CONSERVATIVE

#define lmmp_mul_n_(dst, numa, numb, n)                    \
    if ((n) < MUL_TOOM22_THRESHOLD)                        \
        lmmp_mul_hard_n_((dst), (numa), (numb), (n));      \
    else if ((n) < MUL_TOOM33_THRESHOLD)                   \
        lmmp_mul_toom22_((dst), (numa), (n), (numb), (n)); \
    else if ((n) < MUL_TOOM44_THRESHOLD)                   \
        lmmp_mul_toom33_((dst), (numa), (n), (numb), (n)); \
    else if ((n) < MUL_TOOM6H_THRESHOLD)                   \
        lmmp_mul_toom44_((dst), (numa), (n), (numb), (n)); \
    else if ((n) < MUL_TOOM8H_THRESHOLD)                   \
        lmmp_mul_toom6h_((dst), (numa), (n), (numb), (n)); \
    else if ((n) < MUL_FFT_THRESHOLD)                      \
        lmmp_mul_toom8h_((dst), (numa), (n), (numb), (n)); \
    else                                                   \
        lmmp_mul_fft_((dst), (numa), (n), (numb), (n))

/*
Evaluate in: 0, 1, -1, 2, -2, 4, -4, 1/2, -1/2, 1/4, -1/4
*/

static int toom93_lin_(mp_ptr pp, mp_ptr pm, mp_srcptr x, mp_srcptr y, mp_size_t n) {
    mp_limb_t cy = lmmp_add_n_(pp, x, y, n);
    pp[n] = x[n] + y[n] + cy;
    if (lmmp_cmp_(x, y, n + 1) < 0) {
        lmmp_sub_n_(pm, y, x, n + 1);
        return 1;
    }
    lmmp_sub_n_(pm, x, y, n + 1);
    return 0;
}

static void toom93_shl_(mp_ptr dst, mp_srcptr src, mp_size_t n, unsigned shift) {
    dst[n] = lmmp_shl_(dst, src, n, shift);
}

typedef struct {
    mp_srcptr restrict numb;
    mp_size_t n;
    mp_size_t s;
    mp_size_t t;
    mp_ptr restrict scratch;
    mp_ptr restrict bp;
    mp_ptr b0;
    mp_ptr b1;
    mp_ptr b2;
    mp_ptr even;
    mp_ptr odd;
    mp_ptr tmp0;
    mp_ptr tmp1;
    int bsg;
} toom93_context_t;

static int toom93_eval_point_(
    toom93_context_t* restrict ctx,
    mp_ptr                      pp,
    mp_ptr                      pm,
    unsigned              b0_shift,
    unsigned              b2_shift,
    unsigned              b1_shift
) {
    mp_limb_t cy;

    toom93_shl_(ctx->tmp0, ctx->b0, ctx->n, b0_shift);
    toom93_shl_(ctx->tmp1, ctx->b2, ctx->n, b2_shift);
    cy = lmmp_add_n_(ctx->even, ctx->tmp0, ctx->tmp1, ctx->n);
    ctx->even[ctx->n] = ctx->tmp0[ctx->n] + ctx->tmp1[ctx->n] + cy;
    toom93_shl_(ctx->odd, ctx->b1, ctx->n, b1_shift);
    return toom93_lin_(pp, pm, ctx->even, ctx->odd, ctx->n);
}

/* Cache the five b-side ± pairs.  The even and odd parts are scaled so that
   the total scale of A(x)B(x) matches the degree-10 interpolation contract. */
static int toom93_eval_b_(toom93_context_t* restrict ctx) {
    mp_size_t n = ctx->n;
    mp_size_t t = ctx->t;
    mp_ptr temp = ctx->scratch;
    mp_ptr bp = ctx->bp;
    mp_ptr bp1 = bp;
    mp_ptr bm1 = bp1 + n + 1;
    mp_ptr bp2 = bm1 + n + 1;
    mp_ptr bm2 = bp2 + n + 1;
    mp_ptr bp4 = bm2 + n + 1;
    mp_ptr bm4 = bp4 + n + 1;
    mp_ptr bph1 = bm4 + n + 1;
    mp_ptr bmh1 = bph1 + n + 1;
    mp_ptr bph2 = bmh1 + n + 1;
    mp_ptr bmh2 = bph2 + n + 1;
    int bsg;

    ctx->b0 = temp;
    ctx->b1 = ctx->b0 + n + 1;
    ctx->b2 = ctx->b1 + n + 1;
    ctx->even = ctx->b2 + n + 1;
    ctx->odd = ctx->even + n + 1;
    ctx->tmp0 = ctx->odd + n + 1;
    ctx->tmp1 = ctx->tmp0 + n + 1;

    lmmp_copy(ctx->b0, ctx->numb, n);
    ctx->b0[n] = 0;
    lmmp_copy(ctx->b1, ctx->numb + n, n);
    ctx->b1[n] = 0;
    lmmp_copy(ctx->b2, ctx->numb + 2 * n, t);
    lmmp_zero(ctx->b2 + t, n - t);
    ctx->b2[n] = 0;

    bsg = toom93_eval_point_(ctx, bp1, bm1, 0, 0, 0) & 1;
    bsg |= (toom93_eval_point_(ctx, bp2, bm2, 0, 2, 1) & 1) << 1;
    bsg |= (toom93_eval_point_(ctx, bp4, bm4, 0, 4, 2) & 1) << 2;
    bsg |= (toom93_eval_point_(ctx, bph1, bmh1, 2, 0, 1) & 1) << 3;
    bsg |= (toom93_eval_point_(ctx, bph2, bmh2, 4, 0, 2) & 1) << 4;
    return bsg;
}

static void toom93_pass_(
    mp_ptr                  restrict  dst,
    mp_srcptr               restrict numa,
    const toom93_context_t* restrict  ctx
) {
    mp_srcptr numb = ctx->numb;
    mp_size_t n = ctx->n;
    mp_size_t s = ctx->s;
    mp_size_t t = ctx->t;
    mp_ptr restrict scratch = ctx->scratch;
    mp_srcptr bp = ctx->bp;
    int bsg = ctx->bsg;
    int sign;
    mp_ptr gp = scratch + 9 * n + 3;
    mp_srcptr bp1 = bp;
    mp_srcptr bm1 = bp1 + n + 1;
    mp_srcptr bp2 = bm1 + n + 1;
    mp_srcptr bm2 = bp2 + n + 1;
    mp_srcptr bp4 = bm2 + n + 1;
    mp_srcptr bm4 = bp4 + n + 1;
    mp_srcptr bph1 = bm4 + n + 1;
    mp_srcptr bmh1 = bph1 + n + 1;
    mp_srcptr bph2 = bmh1 + n + 1;
    mp_srcptr bmh2 = bph2 + n + 1;

#define r4 (dst + 3 * n)
#define r2 (dst + 7 * n)
#define r5 (scratch)
#define r3 (scratch + 3 * n + 1)
#define r1 (scratch + 6 * n + 2)
#define v0 (dst + 7 * n)
#define v2 (dst + 9 * n + 2)
#define wsi (scratch + 9 * n + 3)

    sign = (lmmp_toom_eval_pm2rexp_(v2, v0, 8, numa, n, s, 1, gp) & 1) ^ ((bsg >> 3) & 1);
    lmmp_mul_n_(dst, v0, bmh1, n + 1);
    lmmp_mul_n_(r5, v2, bph1, n + 1);
    lmmp_toom_couple_handling_(r5, 2 * n + 1, dst, sign, n, 1, 0);

    sign = (lmmp_toom_eval_pm1_(v2, v0, 8, numa, n, s, gp) & 1) ^ (bsg & 1);
    lmmp_mul_n_(dst, v0, bm1, n + 1);
    lmmp_mul_n_(r3, v2, bp1, n + 1);
    lmmp_toom_couple_handling_(r3, 2 * n + 1, dst, sign, n, 0, 0);

    sign = (lmmp_toom_eval_pm2exp_(v2, v0, 8, numa, n, s, 2, gp) & 1) ^ ((bsg >> 2) & 1);
    lmmp_mul_n_(dst, v0, bm4, n + 1);
    lmmp_mul_n_(r1, v2, bp4, n + 1);
    lmmp_toom_couple_handling_(r1, 2 * n + 1, dst, sign, n, 2, 4);

    sign = (lmmp_toom_eval_pm2rexp_(v2, v0, 8, numa, n, s, 2, gp) & 1) ^ ((bsg >> 4) & 1);
    lmmp_mul_n_(dst, v0, bmh2, n + 1);
    lmmp_mul_n_(r4, v2, bph2, n + 1);
    lmmp_toom_couple_handling_(r4, 2 * n + 1, dst, sign, n, 2, 0);

    sign = (lmmp_toom_eval_pm2_(v2, v0, 8, numa, n, s, gp) & 1) ^ ((bsg >> 1) & 1);
    lmmp_mul_n_(dst, v0, bm2, n + 1);
    lmmp_mul_n_(r2, v2, bp2, n + 1);
    lmmp_toom_couple_handling_(r2, 2 * n + 1, dst, sign, n, 1, 2);

#undef v0
#undef v2

    lmmp_mul_n_(dst, numa, numb, n);
    lmmp_toom_interp12_(dst, r1, r3, r5, n, s + t, 0, wsi);

#undef r1
#undef r2
#undef r3
#undef r4
#undef r5
#undef wsi
}

void lmmp_mul_toom93_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(nb >= 20);
    mp_size_t n = 1 + ((nb - 1) / 3);
    mp_size_t s = na - 8 * n;
    mp_size_t t = nb - 2 * n;
    lmmp_param_assert(8 * n < na && na <= 9 * n);
    lmmp_param_assert(2 * n < nb && nb <= 3 * n);
    lmmp_param_assert(0 < s && s <= n);
    lmmp_param_assert(0 < t && t <= n);

    TEMP_DECL;
    mp_ptr restrict scratch = TALLOC_TYPE(12 * n + 6, mp_limb_t);
    mp_ptr restrict bp = TALLOC_TYPE(10 * n + 10, mp_limb_t);
    toom93_context_t ctx = {
        .numb    = numb,
        .n       = n,
        .s       = s,
        .t       = t,
        .scratch = scratch,
        .bp      = bp
    };
    ctx.bsg = toom93_eval_b_(&ctx);
    toom93_pass_(dst, numa, &ctx);
    TEMP_FREE;
}

static void toom93_cache_init_(
    mp_ptr            restrict  dst,
    mp_srcptr         restrict numa,
    toom93_context_t* restrict  ctx
) {
    ctx->bsg = toom93_eval_b_(ctx);
    toom93_pass_(dst, numa, ctx);
}

static void toom93_cache_(
    mp_ptr                  restrict  dst,
    mp_srcptr               restrict numa,
    const toom93_context_t* restrict  ctx
) {
    toom93_pass_(dst, numa, ctx);
}

void lmmp_mul_toom93_unbalance_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_size_t            na,
    mp_srcptr restrict numb,
    mp_size_t            nb
) {
    lmmp_param_assert(na >= 3 * nb);
    lmmp_param_assert(nb >= 20);

    TEMP_DECL;
    mp_limb_t* restrict ws = SALLOC_TYPE(nb, mp_limb_t);
    toom93_context_t ctx;
    ctx.numb = numb;
    ctx.n = 1 + ((nb - 1) / 3);
    ctx.s = 3 * nb - 8 * ctx.n;
    ctx.t = nb - 2 * ctx.n;
    ctx.scratch = BALLOC_TYPE(12 * ctx.n + 6, mp_limb_t);
    ctx.bp = BALLOC_TYPE(10 * ctx.n + 10, mp_limb_t);

    toom93_cache_init_(dst, numa, &ctx);
    dst += 3 * nb;
    numa += 3 * nb;
    na -= 3 * nb;
    lmmp_copy(ws, dst, nb);
    while (na >= 3 * nb) {
        toom93_cache_(dst, numa, &ctx);
        if (lmmp_add_n_(dst, dst, ws, nb))
            lmmp_inc(dst + nb);
        dst += 3 * nb;
        numa += 3 * nb;
        na -= 3 * nb;
        lmmp_copy(ws, dst, nb);
    }
    if (na > 0) {
        if (na >= nb)
            lmmp_mul_(dst, numa, na, numb, nb);
        else
            lmmp_mul_(dst, numb, nb, numa, na);
        if (lmmp_add_n_(dst, dst, ws, nb))
            lmmp_inc(dst + nb);
    }
    TEMP_FREE;
}

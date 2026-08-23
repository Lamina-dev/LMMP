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

#include "../../../include/lammp/impl/inlines.h"
#include "../../../include/lammp/impl/longlong.h"
#include "../../../include/lammp/impl/log2_exp2.h"
#include "../../../include/lammp/impl/tmp_alloc.h"
#include "../../../include/lammp/numth.h"
#include "../../../include/lammp/lmmpn.h"


static inline void lmmp_cube_3_(mp_ptr restrict dst, mp_limb_t a) {
    mp_limb_t t[2];
    lmmp_mullh_(a, a, t);
    lmmp_mullh_(t[0], a, dst);
    lmmp_mullh_(t[1], a, t);
    dst[1] += t[0];
    dst[2] = t[1] + (dst[1] < t[0] ? 1 : 0);
}

mp_limb_t lmmp_cbrtapprox_3_(mp_limb_t a0, mp_limb_t a1, mp_limb_t a2) {
    lmmp_param_assert(a1 > 0);
    mp_limb_t x[2];
    /* exact high 65 bits */
    mp_limb_t a_hi;
    mp_bitcnt_t bits;
    if (a2 == 0) {
        mp_bitcnt_t a1_bits = lmmp_limb_bits_(a1);
        bits = LIMB_BITS + a1_bits;
        a1_bits--;
        if (a1_bits == 0)
            a_hi = a0;
        else
            a_hi = (a1 << (LIMB_BITS - a1_bits)) | (a0 >> a1_bits);
    } else {
        mp_bitcnt_t a2_bits = lmmp_limb_bits_(a2);
        bits = LIMB_BITS * 2 + a2_bits;
        a2_bits--;
        if (a2_bits == 0)
            a_hi = a1;
        else
            a_hi = (a2 << (LIMB_BITS - a2_bits)) | (a1 >> a2_bits);
    }
    lmmp_debug_assert(bits >= 65);

    x[1] = bits - 1;
    x[0] = log2_fixed_64(a_hi);

    mp_limb_t rem = lmmp_div_1_(x, x, 2, 3);
    if (2 * rem >= 3) // round
        lmmp_inc(x);

    mp_bitcnt_t shift = x[1];
    x[0] = exp2_fixed_64(x[0]);

    lmmp_debug_assert(shift <= 64);
    mp_limb_t r;
    if (shift == 64)
        r = LIMB_MAX;
    else
        r = (x[0] >> (64 - shift)) | (1ULL << shift);

    // log2/exp2 固定精度近似可能高估 1；高估时回退一档，
    // 低估 1 仍然满足 floor(cbrt(x))-[0|1] 的接口语义。
    mp_limb_t t[3], a[3] = {a0, a1, a2};
    lmmp_cube_3_(t, r);
    if (lmmp_cmp_(t, a, 3) > 0)
        --r;
    return r;
}

mp_limb_t lmmp_cbrt_3_(mp_limb_t a0, mp_limb_t a1, mp_limb_t a2) {
    lmmp_param_assert(a1 > 0);

    mp_limb_t r = lmmp_cbrtapprox_3_(a0, a1, a2);
    if (r == LIMB_MAX)
        return LIMB_MAX;
    mp_limb_t t[3], a[3] = {a0, a1, a2};

    // 近似值可能低估或高估 1（以实测为准，不依赖理论单向偏差）。
    lmmp_cube_3_(t, r);
    if (lmmp_cmp_(t, a, 3) > 0)
        return r - 1;

    lmmp_cube_3_(t, r + 1);
    if (lmmp_cmp_(t, a, 3) <= 0)
        return r + 1;
    return r;
}

/*
        A     = Ah * B^(3*lo) + Al

        Ahr   = floor(Ah^(1/3))
        rk    = Ah - Ahr^3
        x_k   = Ahr * B^lo

        x_k+1 = (2*x_k + A / x_k^2 ) / 3
              = x_k + (A / x_k^2 - x_k) / 3
              = x_k + (A - x_k^3) / 3 * x_k^2
              = Ahr * B^lo + (rk * B^(3*lo) + Al) / 3 * x_k^2
              = Ahr * B^lo + Alr

        let  Alr = (rk * B^(3*lo) + Al) / 3 * x_k^2, R = (rk * B^(3*lo) + Al) mod 3 * x_k^2
        such that  (rk * B^(3*lo) + Al) = Alr * 3 * x_k^2 + R
                                          ┌───────────────────────────────────────────────────────────────────────┐
                                        = |Alr * 3 * Ahr^2*B^(2*lo) + R = R_correct + (Alr-1) * 3 * Ahr^2*B^(2*lo)|
                                          └───────────────────────────────────────────────────────────────┬───────┘
        r_k+1 = A - x_k+1^3                                                                               |
              = Ah*B^(3*lo) + Al - Ahr^3*B^(3*lo) - 3*Alr*Ahr^2*B^(2*lo) - 3*Ahr*Alr^2*B^lo - Alr^3       |
              = r_k * B^(3*lo) + Al - 3*Alr*Ahr^2*B^(2*lo) - 3*Ahr*Alr^2*B^lo - Alr^3                     |
              = R - 3*Ahr*Alr^2*B^lo - Alr^3                                                              |
                                                                                                          |
        Alr is either correct or 1 too big. However, in practice, we have found that Alr can be           |
        overestimated by up to 2, and at this point, it can only be Alr = B^lo+1, which is different      |
        from the square root (where the square root Alr is at most B ^ lo)                                |
                                                       ┌────────────────────────────────────────────────┐ |
        r_k+1 = R - 3*Ahr*(Alr-1)^2*B^lo - (Alr-1)^3 + | 3*Alr*Ahr^2*B^(2*lo) - 3*(Alr-1)*Ahr^2*B^(2*lo)├─┘
              = R - 3*Ahr*Alr^2*B^lo - Alr^3           └────────────────────────────────────────────────┘
(adjust)      + 3*Ahr^2*B^(2*lo) + 6*Ahr*Alr*B^lo - 3*Ahr*B^lo + 3*Alr^2 - 3*Alr + 1
*/

// 此阈值是为了保证 cbrt(A)^2 >= B^2/2
// 实际值大约 0x5A827999FCEF3242
#define CBRT_DIVIDE_MIN (0x6000000000000000ull)

void lmmp_cbrt_divide_(mp_ptr restrict dst, mp_ptr restrict numa, mp_size_t ns, mp_ptr restrict tp, int calr) {
    lmmp_param_assert(ns > 0);
    lmmp_param_assert(numa != NULL && dst != NULL && tp != NULL);
    lmmp_param_assert(numa[3 * ns - 1] >= CBRT_DIVIDE_MIN);
    if (ns == 1) {
        dst[0] = lmmp_cbrt_3_(numa[0], numa[1], numa[2]);
        if (calr) {
            lmmp_cube_3_(tp, dst[0]);
            lmmp_sub_n_(numa, numa, tp, 3);
        }
    } else {
        mp_size_t lo = ns / 2, hi = ns - lo;
#define Ahr     (dst + lo)             // [dst+lo,              hi]
#define rk      (numa + 3 * lo)        // [numa+3*lo,       2*hi+1]
#define R       (numa)                 // [numa,            2*ns+1]
#define Ahr2    (tp)                   // [tp,                2*hi]
#define Alr     (tp + 2 * hi)          // [tp + 2*hi,         lo+1]
#define Alr2    (tp + 2 * hi + lo)     // [tp + 2*hi+lo,      2*lo]
#define scratch (tp + 2 * hi + 3 * lo) // [tp + 2*hi+3*lo, hi+2*lo]

        lmmp_cbrt_divide_(Ahr, rk, hi, tp, 1);

        lmmp_sqr_(Ahr2, Ahr, hi);

        /*
        A / (3*x^2) = A / 3 / x^2
        A % (3*x^2) = 3 * (A / 3 % x^2) + A % 3
        */
        mp_limb_t r = lmmp_div_1_(rk - lo, rk - lo, hi + 1 + ns, 3);
        mp_limb_t qh = lmmp_div_s_(Alr, rk - lo, hi + 1 + ns, Ahr2, 2 * hi);
        lmmp_debug_assert(qh == 0);
        (rk - lo)[2 * hi] = lmmp_mul_1_(rk - lo, rk - lo, 2 * hi, 3);
        lmmp_inc_1(rk - lo, r);
        /*
            我们根据 cbrt(A/B^3) == floor(cbrt(A)/B) 可以知道，如果Alr正确结果
            必定被限制在B^lo以内，其至多高估1，因此Alr的最高位必定为0或1，而为1时，
            即代表此时结果已经高估。
        */
        mp_limb_t adj = Alr[lo];
        lmmp_debug_assert(adj == 0 || adj == 1);
        if (adj > 0) {
            // Alr[0] 仅可能为0或1，分别表示高估了1或2
            lmmp_debug_assert(Alr[0] == 0 || Alr[0] == 1);
            lmmp_fill(dst, 0, lo, LIMB_MAX);
            if (calr == 0) return;
            /*
            x_k+1 = Ahr * B^lo + Alr
                  = Ahr * B^lo + B^lo - 1

            r_k+1 = R - 3*Ahr*(B^lo-1)^2*B^lo - (B^lo-1)^3 + 3*Alr*Ahr^2*B^(2*lo) - 3*(Alr-adj)*Ahr^2*B^(2*lo)
                  = R - 3*Ahr*B^(3*lo) + 6*Ahr*B^(2*lo) - 3*Ahr*B^lo + 3*adj*Ahr^2*B^(2*lo)
                    - B^(3*lo) + 3*B^(2*lo) - 3*B^lo + 1
            */
            adj += Alr[0];

            // - 3*Ahr*B^(3*lo)
            mp_limb_t cy = lmmp_submul_1_(R + 3 * lo, Ahr, hi, 3);
            r = lmmp_sub_1_(R + 2 * lo + ns, R + 2 * lo + ns, ns + 1 - 2 * lo, cy);

            // + 6*Ahr*B^(2*lo)
            cy = lmmp_addmul_1_(R + 2 * lo, Ahr, hi, 6);
            r -= lmmp_add_1_(R + ns + lo, R + ns + lo, hi + 1, cy);

            // - 3*Ahr*B^lo
            cy = lmmp_submul_1_(R + lo, Ahr, hi, 3);
            r += lmmp_sub_1_(R + ns, R + ns, ns + 1, cy);

            // + 3*adj*Ahr^2*B^(2*lo)
            cy = lmmp_addmul_1_(R + 2 * lo, Ahr2, 2 * hi, 3 * adj);
            (R + 2 * ns)[0] += cy;
            r -= (R + 2 * ns)[0] < cy;

            // - B^(3*lo)
            r -= lmmp_sub_1_(R + 3 * lo, R + 3 * lo, 2 * ns + 1 - 3 * lo, 1);

            // + 3*B^(2*lo)
            r -= lmmp_add_1_(R + 2 * lo, R + 2 * lo, 2 * hi + 1, 3);

            // - 3*B^lo
            r += lmmp_sub_1_(R + lo, R + lo, 2 * ns + 1 - lo, 3);

            // + 1
            r -= lmmp_add_1_(R, R, 2 * ns + 1, 1);

            lmmp_debug_assert(r == 0);
        } else {
            lmmp_sqr_(Alr2, Alr, lo);
            lmmp_mul_(scratch, Alr2, 2 * lo, Alr, lo);
            r = lmmp_sub_(R, R, 2 * ns + 1, scratch, 3 * lo);

            lmmp_mul_(scratch, Alr2, 2 * lo, Ahr, hi);
            mp_limb_t b = lmmp_submul_1_(R + lo, scratch, 2 * lo + hi, 3);
            r += lmmp_sub_1_(R + 2 * lo + ns, R + 2 * lo + ns, ns + 1 - 2 * lo, b);

            if (calr == 0) {
                if (r > 0)
                    lmmp_dec(Alr);
                lmmp_copy(dst, Alr, lo);
                return;
            }
            if (r > 0) {
                // + 3*Alr^2
                mp_limb_t cy = lmmp_addmul_1_(R, Alr2, 2 * lo, 3);
                r -= lmmp_add_1_(R + 2 * lo, R + 2 * lo, 2 * hi + 1, cy);

                // - 3*Alr
                cy = lmmp_submul_1_(R, Alr, lo, 3);
                r += lmmp_sub_1_(R + lo, R + lo, 2 * ns + 1 - lo, cy);

                // + 6*Ahr*Alr*B^lo
                lmmp_mul_(scratch, Ahr, hi, Alr, lo);
                cy = lmmp_addmul_1_(R + lo, scratch, ns, 6);
                r -= lmmp_add_1_(R + ns + lo, R + ns + lo, ns + 1 - lo, cy);

                // + 3*Ahr^2*B^(2*lo)
                cy = lmmp_addmul_1_(R + 2 * lo, Ahr2, 2 * hi, 3);
                // r -= lmmp_add_1_(R + 2 * ns, R + 2 * ns, 1, cy);
                (R + 2 * ns)[0] += cy;
                r -= (R + 2 * ns)[0] < cy;

                // + 1
                r -= lmmp_add_1_(R, R, 2 * ns + 1, 1);

                // - 3*Ahr*B^lo
                cy = lmmp_submul_1_(R + lo, Ahr, hi, 3);
                r += lmmp_sub_1_(R + ns, R + ns, ns + 1, cy);

                lmmp_debug_assert(r == 0);
                lmmp_dec(Alr);
            }
            lmmp_copy(dst, Alr, lo);
        }
    }
#undef Ahr
#undef rk
#undef R
#undef Ahr2
#undef Alr
#undef Alr2
#undef scratch
}

#if 0
/*

    B^(3*ns) // [numa,3*ns]^(2/3)

    A     = Ah * B^(3*lo) + Al

    Ahr   = B^(nf+3*hi) / Ah^(2/3)
    x_k   = Ahr * B^lo

    x_k+1 = x_k + x_k/3 - A^2 * x_k^4 / 3 / B^(9*na+3*nf)

*/

#define INVCBRT_MIN 0xa000000000000000ull

void lmmp_invcbrt_newton_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa != NULL && dst != NULL);
    lmmp_param_assert(numa[3 * na - 1] >= INVCBRT_MIN);

    mp_size_t ns = na + nf;
    if (ns == 1) {
        mp_limb_t a_sqr[6], a_sqrcbrt[2], tp[9];
        lmmp_sqr_basecase_(a_sqr, numa, 3);
        lmmp_cbrt_divide_(a_sqrcbrt, a_sqr, 2, tp, 0);
        lmmp_zero(tp, 3);
        tp[3] = 1;

        lmmp_div_2_s_(dst, tp, 4, a_sqrcbrt);
    } else {

    }
}

#endif
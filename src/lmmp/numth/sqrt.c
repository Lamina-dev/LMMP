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
#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/lmmpn.h"
#include "../../../include/lmmp/numth.h"


/*
        A     = Ah * B^(2*lo) + Al

        Ahr   = floor(Ah^(1/2))
        rk    = Ah - Ahr^2
        x_k   = Ahr * B^lo

        x_k+1 = (x_k + A / x_k ) / 2
              = x_k + (A / x_k - x_k) / 2
              = x_k + (A - x_k^2) / 2 * x_k
              = Ahr * B^lo + (rk * B^(2*lo) + Al) / 2 * x_k
              = Ahr * B^lo + Alr

        let  Alr = (rk * B^(2*lo) + Al) / 2 * x_k, R = (rk * B^(2*lo) + Al) mod 2 * x_k
        such that  (rk * B^(2*lo) + Al) = Alr * 2 * x_k + R
                                          ┌───────────────────────────────────────────────────────────┐
                                        = |Alr * 2 * Ahr*B^lo + R = R_correct + (Alr-1) * 2 * Ahr*B^lo|
                                          └───────────────────────────────────────────────────┬───────┘
                                                                                              |
        r_k+1 = A - x_k+1^2                                                                   |
              = Ah*B^(2*lo) + Al - Ahr^2*B^(2*lo) - 2*Alr*Ahr*B^lo - Alr^2                    |
              = r_k * B^(2*lo) + Al - 2*Alr*Ahr*B^lo - Alr^2                                  |
              = R - Alr^2                                                                     |
                                                                                              |
        Alr is either correct or 1 too big,                                                   |
                                ┌────────────────────────────────────┐                        |
        r_k+1 = R - (Alr-1)^2 + | 2*Alr*Ahr*B^lo - 2*(Alr-1)*Ahr*B^lo├────────────────────────┘
              = R - Alr^2       └────────────────────────────────────┘
(adjust)      + 2*Ahr*B^lo + 2*Alr - 1
*/

void lmmp_sqrt_divide_(mp_ptr restrict dst, mp_ptr restrict numa, mp_size_t ns, mp_ptr restrict tp, int calr) {
    lmmp_param_assert(ns > 0);
    lmmp_param_assert(numa != NULL && dst != NULL && tp != NULL);
    lmmp_param_assert(numa[2 * ns - 1] >= LIMB_B_4);
    if (ns == 1) {
        dst[0] = lmmp_sqrt_2_(numa, numa);
    } else {
        mp_size_t lo = ns / 2, hi = ns - lo;
#define Ahr  (dst + lo)         // [dst+lo,          hi]
#define rk   (numa + 2 * lo)    // [numa+2*lo,     hi+1]
#define R    (numa)             // [numa,          ns+1]
#define Alr  (tp)               // [tp,            lo+1]
#define Alr2 (tp + lo + 1)      // [tp + lo+1,     2*lo]

        lmmp_sqrt_divide_(Ahr, rk, hi, tp, 1);

        /*
        A / (2*x) = A / 2 / x
        A % (2*x) = 2 * (A / 2 % x) + A % 2
        */
        mp_limb_t r = lmmp_shr_(rk - lo, rk - lo, ns + 1, 1);
        mp_limb_t qh = lmmp_div_s_(Alr, rk - lo, ns + 1, Ahr, hi);
        lmmp_debug_assert(qh == 0);
        (rk - lo)[hi] = lmmp_shl_c_(rk - lo, rk - lo, hi, 1, r >> (LIMB_BITS - 1));
        /*
            我们根据 sqrt(A/B^2) == floor(sqrt(A)/B) 可以知道，如果Alr正确结果
            必定被限制在B^lo以内，其至多高估1，因此Alr的最高位必定为0或1，而为1时，
            即代表此时结果已经高估。
        */
        mp_limb_t adj = Alr[lo];
        lmmp_debug_assert(adj == 0 || adj == 1);
        if (adj > 0) {
            lmmp_debug_assert(Alr[0] == 0);
            lmmp_fill(dst, 0, lo, LIMB_MAX);
            if (calr == 0) return;
            /*
            x_k+1 = Ahr * B^lo + Alr
                  = Ahr * B^lo + B^lo - 1

            r_k+1 = R - (B^lo-1)^2 + 2*Alr*Ahr*B^lo - 2*(Alr-adj)*Ahr*B^lo
                  = R - B^(2*lo) + 2*B^lo - 1 + 2*adj*Ahr*B^lo
            */

            // + 2*adj*Ahr*B^lo
            mp_limb_t cy = lmmp_addmul_1_(R + lo, Ahr, hi, 2 * adj);
            (R + ns)[0] += cy;
            r = (R + ns)[0] < cy;

            // - 1
            r -= lmmp_sub_1_(R, R, ns + 1, 1);

            // + 2*B^lo
            r += lmmp_add_1_(R + lo, R + lo, hi + 1, 2);

            // - B^(2*lo)
            r -= lmmp_sub_1_(R + 2 * lo, R + 2 * lo, hi + 1 - lo, 1);
            lmmp_debug_assert(r == 0);
        } else {
            lmmp_sqr_(Alr2, Alr, lo);
            mp_limb_t b = lmmp_sub_(R, R, ns + 1, Alr2, 2 * lo);
            if (calr == 0) {
                if (b > 0)
                    lmmp_dec(Alr);
                lmmp_copy(dst, Alr, lo);
                return;
            }
            if (b > 0) {
                // + 2*Ahr*B^lo
                mp_limb_t cy = lmmp_addshl1_n_(R + lo, R + lo, Ahr, hi);
                (R + lo)[hi] += cy;
                b -= (R + lo)[hi] < cy;

                // + 2*Alr
                cy = lmmp_addshl1_n_(R, R, Alr, lo);
                b -= lmmp_add_1_(R + lo, R + lo, hi + 1, cy);

                // - 1
                b += lmmp_sub_1_(R, R, ns + 1, 1);

                lmmp_debug_assert(b == 0);
                lmmp_dec(Alr);
            }
            lmmp_copy(dst, Alr, lo);
        }
    }
#undef Ahr
#undef rk
#undef R
#undef Alr
#undef Alr2
}

void lmmp_invsqrt_newton_(mp_ptr restrict dstis, mp_size_t ns, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(ns >= 3);
    lmmp_param_assert(na > 0);
    lmmp_param_assert(numa != NULL && dstis != NULL);
    lmmp_param_assert(numa[na - 1] >= LIMB_B_4);
    mp_size_t nr = ns, namax = na, mn;
    mp_size_t sizes[LIMB_BITS], *sizp = sizes;

    do {
        *sizp = nr;
        nr = (nr >> 1) + 1;
        ++sizp;
    } while (nr > 2);

    numa += na;
    dstis += ns;

    // nr=2
    // i2=floor((B^5-1)/(1+floor(sqrt(x*B^4))))
    mp_limb_t numa2[6], sval[3], tp[4];
    lmmp_zero(numa2, 4);
    numa2[5] = numa[-1];
    if (na > 1)
        numa2[4] = numa[-2];
    else
        numa2[4] = 0;

    lmmp_sqrt_divide_(sval, numa2, 3, tp, 0);
    lmmp_inc(sval);

    for (mp_size_t i = 0; i < 5; ++i) numa2[i] = LIMB_MAX;
    dstis[0] = lmmp_div_s_(dstis - 2, numa2, 5, sval, 3);

    TEMP_DECL;
    mp_limb_t alloc_size = na + 2 * ns + 6;
    mp_ptr restrict xp = TALLOC_TYPE(alloc_size, mp_limb_t);
    do {
        na = *--sizp;

        // ar = 0:[numa-nr,nr]
        // an = 0:[numa-na,na]
        // ir = 1:[dst-nr,nr] = floor(B^(3*nr/2)/sqrt(ar)) - [0|1]
        //  d = B^(na+2*nr)-an*ir*ir
        //  -4*B^(na+nr) < d < 4*B^(na+nr)

        mp_size_t naz = LMMP_MIN(na, namax);
        // mp_size_t zeros = na - naz;
        mp_size_t nsqr, nres = naz + nr + 1;
        mp_ptr dp = xp + 2 * nr + 1, dip = xp + nr + 1;
        int cmod;  // 1=mod b^mn-1, 0=mod b^(naz+nr+1)
        int sign;  // 1:d<0, 0:d>=0
        mn = lmmp_fft_next_size_(nres);

        // ir^2
        if (2 * SQRT_NEWTON_MODM_THRESHOLD + mn >= nr * 2 + 1) {
            cmod = 0;
            lmmp_sqr_(xp, dstis - nr, nr + 1);
            nsqr = 2 * nr + 1;
        } else {
            cmod = 1;
            lmmp_sqr_mersenne_(xp, mn, dstis - nr, nr + 1);
            nsqr = mn;
        }

        // ir^2*an
        if (naz < SQRT_NEWTON_MODM_THRESHOLD || naz * 8 < nsqr || mn >= nsqr + naz) {
            if (cmod == 0)
                nsqr = LMMP_MIN(nsqr, nres);
            lmmp_mul_(dp, xp, nsqr, numa - naz, naz);
            if (cmod == 1) {
                if (lmmp_add_(dp, dp, mn, dp + mn, naz))
                    lmmp_inc(dp);
            }
        } else {
            if (nsqr > mn) {  // cmod==0
                if (lmmp_add_(xp, xp, mn, xp + mn, nsqr - mn))
                    lmmp_inc(xp);
            }
            lmmp_mul_mersenne_(dp, mn, xp, nsqr, numa - naz, naz);
            cmod = 1;
        }

        if (cmod == 1) {
            // naz+nr < mn <= naz+2*nr
            //[dp,mn] -= B^(naz+2*nr) mod (B^mn-1)
            dp[mn] = 1;
            lmmp_dec(dp + naz + 2 * nr - mn);
            if (dp[mn] == 0)
                lmmp_dec(dp);
        }

        if (dp[nres - 1] > 3) {  //-d<0
            if (cmod == 0)
                lmmp_dec(dp);  // for neg to not
            // else (neg to not) compensate (mod transfer)
            dp += naz;
            lmmp_shlnot_(xp, dp + 1, nr, LIMB_BITS - 1);
            xp[0] ^= dp[0] >> 1;
            xp[nr] = ~dp[nr] >> 1;
            sign = 0;
        } else {  //-d>0
            lmmp_shr_(xp, dp + naz, nr + 1, 1);
            if ((dp[naz] & 1) || !lmmp_zero_q_(dp, naz))
                lmmp_inc(xp);
            sign = 1;
        }

        lmmp_mul_n_(dip, xp, dstis - nr, nr + 1);

        if (sign) {
            if (lmmp_zero_q_(dip, 3 * nr - na)) {
                // a limit for dec
                dip[2 * nr + 1] = 1;
                lmmp_dec(dip + 3 * nr - na);
            }
            lmmp_not_(dstis - na, dip + 3 * nr - na, na - nr);
            lmmp_dec_1(dstis - nr, dip[2 * nr] + 1);
        } else {
            lmmp_copy(dstis - na, dip + 3 * nr - na, na - nr);
            lmmp_inc_1(dstis - nr, dip[2 * nr]);
        }

        nr = na;
    } while (sizp != sizes);
    TEMP_FREE;
}

void lmmp_sqrt_newton_(mp_ptr dsts, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(nf >= 2);
    lmmp_param_assert(numa != NULL && dsts != NULL);
    mp_limb_t high = numa[na - 1];
    int nsh = lmmp_leading_zeros_(high) / 2;
    mp_size_t ns = na / 2 + 1 + nf;

    TEMP_DECL;
    mp_limb_t alloc_size = (nsh ? na : 0) + ns + 1;
    mp_ptr tp = TALLOC_TYPE(alloc_size, mp_limb_t), numa2;
    if (nsh) {
        numa2 = tp;
        lmmp_shl_(numa2, numa, na, nsh * 2);
        tp += na;
    } else
        numa2 = (mp_ptr)numa;

    lmmp_invsqrt_newton_(tp, ns, numa2, na);

    mp_ptr restrict msqr = TALLOC_TYPE(na + ns + 1, mp_limb_t);

    if (ns + 1 > na)
        lmmp_mul_(msqr, tp, ns + 1, numa2, na);
    else
        lmmp_mul_(msqr, numa2, na, tp, ns + 1);

    mp_limb_t cceil;
    if (na & 1) {
        nsh += LIMB_BITS / 2;
        lmmp_shr_(dsts, msqr + na, ns, nsh);
        cceil = msqr[na] >> (nsh - 1);
    } else {
        if (nsh) {
            lmmp_shr_(dsts, msqr + na + 1, ns - 1, nsh);
            cceil = msqr[na + 1] >> (nsh - 1);
        } else {
            lmmp_copy(dsts, msqr + na + 1, ns - 1);
            cceil = msqr[na] >> (LIMB_BITS - 1);
        }
        dsts[ns - 1] = 0;
    }

    if (cceil & 1)
        lmmp_inc(dsts);

    TEMP_FREE;
}

void lmmp_sqrt_(mp_ptr dsts, mp_ptr dstr, mp_srcptr numa, mp_size_t na, mp_size_t nf) {
    lmmp_debug_assert(na > 0);
    lmmp_debug_assert(numa[na - 1] > 0);
    mp_limb_t high = numa[na - 1];
    int nsh = lmmp_leading_zeros_(high) / 2;
    mp_size_t nl = na + 2 * nf;
    if (nl == 1) {
        mp_limb_t r;
        mp_limb_t srt = lmmp_sqrt_1_(&r, high << nsh * 2);
        srt >>= nsh;
        dsts[0] = srt;
        if (dstr)
            dstr[0] = high - srt * srt;
    } else if (!dstr && nf >= 10 * na + SQRT_INVNEWTON_THRESHOLD) {
        lmmp_sqrt_newton_(dsts, numa, na, nf);
    } else {
        TEMP_DECL;
        mp_limb_t ns = (nl + 1) / 2;
        mp_ptr restrict numa2 = TALLOC_TYPE(2 * ns, mp_limb_t);
        mp_ptr restrict tp = TALLOC_TYPE(3 * ns / 2 + 1, mp_limb_t);
        lmmp_zero(numa2, 2 * nf);
        if (nsh)
            lmmp_shl_(numa2 + 2 * ns - na, numa, na, nsh * 2);
        else
            lmmp_copy(numa2 + 2 * ns - na, numa, na);
        if (nl & 1) {
            numa2[2 * nf] = 0;
            nsh += LIMB_BITS / 2;
        } else {
            dsts[ns] = 0;
        }

        lmmp_sqrt_divide_(dsts, numa2, ns, tp, dstr ? 1: 0);
        if (nsh) {
            /*
                let
                    s = sqrt(A*T^2), r = sqrtrem(A*T^2)
                such that
                    s = T*a + t, where a = sqrt(A) = s // T
                then
                    sqrtrem(A) = (r + 2*T*a*t + t^2) // T^2
                               = (r + 2*t*(s-t) + t^2) // T^2
                               = (r + 2*t*s - t^2) // T^2
            */
            if (dstr) {
                mp_limb_t t = dsts[0] & (((mp_limb_t)1 << nsh) - 1);
                numa2[ns] += lmmp_addmul_1_(numa2, dsts, ns, 2 * t);
                mp_limb_t b = lmmp_submul_1_(numa2, &t, 1, t);
                lmmp_sub_1_(numa2 + 1, numa2 + 1, ns, b);
            }
            lmmp_shr_(dsts, dsts, ns, nsh);
        }
        if (dstr) {
            nsh *= 2;
            if (nsh >= LIMB_BITS) {
                nsh -= LIMB_BITS;
                ++numa2;
            } else
                ++ns;
            if (nsh)
                lmmp_shr_(dstr, numa2, ns, nsh);
            else
                lmmp_copy(dstr, numa2, ns);
        }
        
        TEMP_FREE;
    }
}

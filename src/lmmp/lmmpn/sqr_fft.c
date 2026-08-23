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

#include "../../../include/lmmp/impl/inlines.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/tmp_alloc.h"
#include "../../../include/lmmp/impl/fft_ssa.h"
#include "../../../include/lmmp/lmmpn.h"


static void lmmp_sqr_fermat_recurse_(fft_memstack* ms, mp_ptr* pc1, mp_size_t K0) {
    mp_ptr push_temp_coef = ms->temp_coef;
    mp_size_t rn = ms->lenw;

    if (rn < MUL_FFT_MODF_THRESHOLD) {
        mp_ptr temp_mul = (mp_ptr)lmmp_fft_memstack_(ms, (rn + 1) * 2 * LIMB_BYTES);
        for (mp_size_t i = 0; i < K0; ++i) {
            lmmp_sqr_(temp_mul, pc1[i], rn + 1);

            // 模 B^rn+1 归一化：temp_mul - temp_mul[rn ...]
            mp_limb_t maxc = lmmp_sub_n_(pc1[i], temp_mul, temp_mul + rn, rn) + temp_mul[rn * 2];
            pc1[i][rn] = 0;
            lmmp_inc_1(pc1[i], maxc);
        }
        lmmp_fft_memstack_(ms, 0);
    } else {
        mp_size_t N = rn * LIMB_BITS;
        mp_size_t k = lmmp_fft_best_k_(rn);
        mp_size_t K = ((mp_size_t)1) << k;
        lmmp_debug_assert(!(N & (K - 1)));
        mp_size_t M = N >> k;
        mp_size_t n = 2 * M + k + 2;

        n = (n + LIMB_BITS - 1) & (-LIMB_BITS);
        n = (((n - 1) >> k) + 1) << k;

        ms->lenw = n / LIMB_BITS;
        mp_size_t nlen = ms->lenw + 1;

        ms->temp_coef = (mp_ptr)lmmp_fft_memstack_(ms, (((nlen + 1) << k) + nlen) * LIMB_BYTES);
        mp_ptr *pfca = (mp_ptr*)(ms->temp_coef + nlen);
        for (mp_size_t i = 0; i < K; ++i) pfca[i] = (mp_ptr)(pfca + K) + i * nlen;

        for (mp_size_t j = 0; j < K0; ++j) {
            mp_ptr numa = pc1[j];

            for (mp_size_t i = 0; i < K; ++i) {
                lmmp_fft_extract_coef_(pfca[i], numa, M * i, M + (i == K - 1), ms->lenw);
                if (i > 0)
                    lmmp_fft_shl_coef_(ms, pfca + i, i * n >> k);
            }
            lmmp_fft_(ms, pfca, k, n >> (k - 1));

            // dot product
            lmmp_sqr_fermat_recurse_(ms, pfca, K);

            lmmp_ifft_(ms, pfca, k, n >> (k - 1));

            lmmp_mul_fermat_recombine_(ms, numa, pfca, K, k, n, M, rn);
        }
        lmmp_fft_memstack_(ms, 0);
    }

    ms->temp_coef = push_temp_coef;
    ms->lenw = rn;
}

void lmmp_sqr_fermat_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na) {
    mp_size_t N = rn * LIMB_BITS;
    mp_size_t k = lmmp_fft_best_k_(rn);
    mp_size_t K = ((mp_size_t)1) << k;
    lmmp_debug_assert(!(N & (K - 1)));
    mp_size_t M = N >> k;
    mp_size_t n = 2 * M + k + 2;

    n = (n + LIMB_BITS - 1) & (-LIMB_BITS);
    n = (((n - 1) >> k) + 1) << k;

    fft_memstack msr;
    msr.maxdepth = -1;
    msr.tempdepth = -1;
    msr.lenw = n / LIMB_BITS;
    mp_size_t nlen = msr.lenw + 1;

    msr.temp_coef = (mp_ptr)lmmp_fft_memstack_(&msr, (((nlen + 1) << k) + nlen) * LIMB_BYTES);

    mp_ptr *pfca = (mp_ptr*)(msr.temp_coef + nlen);
    mp_size_t narest = na * LIMB_BITS;

    for (mp_size_t i = 0; i < K; ++i) {
        mp_size_t coeflen;
        pfca[i] = (mp_ptr)(pfca + K) + i * nlen;
        if (narest > 0) {
            coeflen = M + (i == K - 1);
            coeflen = LMMP_MIN(narest, coeflen);
            narest -= coeflen;
            lmmp_fft_extract_coef_(pfca[i], numa, M * i, coeflen, msr.lenw);
            if (i > 0)
                lmmp_fft_shl_coef_(&msr, pfca + i, i * n >> k);
        } else {
            lmmp_zero(pfca[i], nlen);
        }
    }
    lmmp_fft_(&msr, pfca, k, n >> (k - 1));

    lmmp_sqr_fermat_recurse_(&msr, pfca, K);

    lmmp_ifft_(&msr, pfca, k, n >> (k - 1));

    lmmp_mul_fermat_recombine_(&msr, dst, pfca, K, k, n, M, rn);

    if (dst[rn] && !lmmp_zero_q_(dst, rn)) {
        dst[rn] = 0;
        lmmp_dec(dst);
    }

    lmmp_fft_memstack_(&msr, 0);
}

void lmmp_sqr_mersenne_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na) {
    mp_size_t N = rn * LIMB_BITS;
    mp_size_t k = lmmp_fft_best_k_(rn);
    mp_size_t K = ((mp_size_t)1) << k;

    lmmp_debug_assert(!(N & (K - 1)));
    mp_size_t M = N >> k;
    mp_size_t n = 2 * M + k;

    n = (n + LIMB_BITS - 1) & (-LIMB_BITS);
    n = (((n - 1) >> (k - 1)) + 1) << (k - 1);

    fft_memstack msr;
    msr.maxdepth = -1;
    msr.tempdepth = -1;
    msr.lenw = n / LIMB_BITS;
    mp_size_t nlen = msr.lenw + 1;

    msr.temp_coef = (mp_ptr)lmmp_fft_memstack_(&msr, (((nlen + 1) << k) + nlen) * LIMB_BYTES);

    mp_ptr *pfca = (mp_ptr*)(msr.temp_coef + nlen);
    mp_size_t narest = na * LIMB_BITS;

    for (mp_size_t i = 0; i < K; ++i) {
        mp_size_t coeflen;
        pfca[i] = (mp_ptr)(pfca + K) + i * nlen;
        if (narest > 0) {
            coeflen = LMMP_MIN(narest, M);
            narest -= coeflen;
            lmmp_fft_extract_coef_(pfca[i], numa, M * i, coeflen, msr.lenw);
        } else {
            lmmp_zero(pfca[i], nlen);
        }
    }
    lmmp_fft_(&msr, pfca, k, n >> (k - 1));

    lmmp_sqr_fermat_recurse_(&msr, pfca, K);

    lmmp_ifft_(&msr, pfca, k, n >> (k - 1));

    mp_size_t rhead = 0, maxc = 0;
    for (mp_size_t i = 0; i < K; ++i) {
        lmmp_fft_shr_coef_(&msr, pfca + i, k);
        mp_ptr nums = pfca[i];

        if (nums[nlen - 1]) {
            lmmp_dec(nums);
            lmmp_debug_assert(nums[nlen - 1] == 1);
            nums[nlen - 1] = 0;
        }

        mp_size_t roffset = i * M;
        mp_size_t shl = roffset & (LIMB_BITS - 1);
        roffset /= LIMB_BITS;

        if (shl)
            lmmp_shl_(nums, nums, nlen, shl);

        if (i == 0) {
            lmmp_copy(dst, nums, nlen);
            rhead = nlen;
        } else if (roffset + nlen <= rn) {
            lmmp_add_(dst + roffset, nums, nlen, dst + roffset, rhead - roffset);
            rhead = roffset + nlen;
        } else {
            maxc += lmmp_add_(dst + roffset, nums, rn - roffset, dst + roffset, rhead - roffset);
            maxc += lmmp_add_(dst, dst, rn, nums + rn - roffset, nlen + roffset - rn);
            rhead = rn;
        }
    }

    if (!lmmp_add_1_(dst, dst, rn, 1 + maxc))
        lmmp_dec(dst);

    lmmp_fft_memstack_(&msr, 0);
}

void lmmp_sqr_fft_(mp_ptr dst, mp_srcptr numa, mp_size_t na) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(dst != NULL && numa != NULL);
    mp_size_t hn = lmmp_fft_next_size_((na + na + 1) >> 1);
    lmmp_debug_assert(na <= hn);
    mp_ptr tp = ALLOC_TYPE(hn + 1, mp_limb_t);

    lmmp_sqr_mersenne_(dst, hn, numa, na);
    lmmp_sqr_fermat_(tp, hn, numa, na);

    mp_limb_t cy = lmmp_shr1add_nc_(dst, dst, tp, hn, tp[hn]);
    cy <<= LIMB_BITS - 1;
    dst[hn - 1] += cy;
    if (dst[hn - 1] < cy)
        lmmp_inc(dst);

    if (na == hn) {
        cy = tp[hn] + lmmp_sub_n_(dst + hn, dst, tp, hn);
        // cy==1 means [tp,hn+1]!=0, then [dst,hn]!=0
        // cy==2 is impossible since [tp,hn+1] is normalized.
        // so the following dec won't overflow.
        lmmp_dec_1(dst, cy);
    } else {
        cy = lmmp_sub_n_(dst + hn, dst, tp, 2 * na - hn);
        cy = tp[hn] + lmmp_sub_nc_(tp + 2 * na - hn, dst + 2 * na - hn, tp + 2 * na - hn, 2 * hn - (2 * na), cy);
        cy = lmmp_sub_1_(dst, dst, 2 * na, cy);
    }
    lmmp_free(tp);
}

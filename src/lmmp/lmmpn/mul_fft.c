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
#include "../../../include/lmmp/impl/mul_cache.h"
#include "../../../include/lmmp/lmmpn.h"


/**
 * @brief 费马变换乘法递归函数（核心乘法逻辑）
 * @param ms - 内存栈结构体指针
 * @param pc1 - 第一个数的FFT系数数组指针数组
 * @param pc2 - 第二个数的FFT系数数组指针数组
 * @param K0 - FFT块数
 * @warning K0>0
 *          所有系数均已伪归一化（mod B^lenw+1）
 *          nsqr=1表示乘法，nsqr=0表示平方
 */
static void lmmp_mul_fermat_recurse_(fft_memstack* ms, mp_ptr* pc1, mp_ptr* pc2, mp_size_t K0) {
    const int nsqr = 1;
    mp_ptr push_temp_coef = ms->temp_coef;
    mp_size_t rn = ms->lenw;

    if (rn < MUL_FFT_MODF_THRESHOLD) {
        mp_ptr temp_mul = (mp_ptr)lmmp_fft_memstack_(ms, (rn + 1) * 2 * LIMB_BYTES);
        for (mp_size_t i = 0; i < K0; ++i) {
            lmmp_mul_n_(temp_mul, pc1[i], pc2[i], rn + 1);

            // 模 B^rn+1 归一化：temp_mul - temp_mul[rn ...]
            mp_limb_t maxc = lmmp_sub_n_(pc1[i], temp_mul, temp_mul + rn, rn) + temp_mul[rn * 2];
            pc1[i][rn] = 0;
            lmmp_inc_1(pc1[i], maxc);
        }
        lmmp_fft_memstack_(ms, 0);
    } else {
        mp_size_t N = rn * LIMB_BITS;        // 总比特数
        mp_size_t k = lmmp_fft_best_k_(rn);
        mp_size_t K = ((mp_size_t)1) << k;   // FFT块数（2^k）
        lmmp_debug_assert(!(N & (K - 1)));
        mp_size_t M = N >> k;         // 每个块的比特数（N/K）
        mp_size_t n = 2 * M + k + 2;  // 扩展系数长度（保证归一化）

        // 规整n：必须是LIMB_BITS和K的整数倍
        n = (n + LIMB_BITS - 1) & (-LIMB_BITS);
        n = (((n - 1) >> k) + 1) << k;

        ms->lenw = n / LIMB_BITS;
        mp_size_t nlen = ms->lenw + 1;

        ms->temp_coef = (mp_ptr)lmmp_fft_memstack_(ms, (((nlen + 1) << (k + nsqr)) + nlen) * LIMB_BYTES);
        mp_ptr *pfca = (mp_ptr*)(ms->temp_coef + nlen), *pfcb = pfca;
        for (mp_size_t i = 0; i < K; ++i) pfca[i] = (mp_ptr)(pfca + K) + i * nlen;
        pfcb += (nlen + 1) << k;
        for (mp_size_t i = 0; i < K; ++i) pfcb[i] = (mp_ptr)(pfcb + K) + i * nlen;

        for (mp_size_t j = 0; j < K0; ++j) {
            mp_ptr numa = pc1[j];
            mp_ptr numb = pc2[j];

            for (mp_size_t i = 0; i < K; ++i) {
                lmmp_fft_extract_coef_(pfca[i], numa, M * i, M + (i == K - 1), ms->lenw);
                if (i > 0)
                    lmmp_fft_shl_coef_(ms, pfca + i, i * n >> k);
            }
            lmmp_fft_(ms, pfca, k, n >> (k - 1));

            for (mp_size_t i = 0; i < K; ++i) {
                lmmp_fft_extract_coef_(pfcb[i], numb, M * i, M + (i == K - 1), ms->lenw);
                if (i > 0)
                    lmmp_fft_shl_coef_(ms, pfcb + i, i * n >> k);
            }
            lmmp_fft_(ms, pfcb, k, n >> (k - 1));

            // dot product
            lmmp_mul_fermat_recurse_(ms, pfca, pfcb, K);

            lmmp_ifft_(ms, pfca, k, n >> (k - 1));

            lmmp_mul_fermat_recombine_(ms, numa, pfca, K, k, n, M, rn);
        }
        lmmp_fft_memstack_(ms, 0);
    }

    ms->temp_coef = push_temp_coef;
    ms->lenw = rn;
}

void lmmp_mul_fermat_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb) {
    const int nsqr = 1;
    mp_size_t N = rn * LIMB_BITS;         // 结果总比特数
    mp_size_t k = lmmp_fft_best_k_(rn);
    mp_size_t K = ((mp_size_t)1) << k;    // FFT块数（2^k）
    lmmp_debug_assert(!(N & (K - 1)));
    mp_size_t M = N >> k;         // 每个块的比特数
    mp_size_t n = 2 * M + k + 2;  // 扩展系数长度

    n = (n + LIMB_BITS - 1) & (-LIMB_BITS);
    n = (((n - 1) >> k) + 1) << k;

    // 初始化内存栈
    fft_memstack msr;
    msr.maxdepth = -1;
    msr.tempdepth = -1;
    msr.lenw = n / LIMB_BITS;
    mp_size_t nlen = msr.lenw + 1;

    msr.temp_coef = (mp_ptr)lmmp_fft_memstack_(&msr, (((nlen + 1) << (k + nsqr)) + nlen) * LIMB_BYTES);

    mp_ptr *pfca = (mp_ptr*)(msr.temp_coef + nlen), *pfcb = pfca;
    mp_size_t narest = na * LIMB_BITS, nbrest = nb * LIMB_BITS;

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

    pfcb += (nlen + 1) << k;
    for (mp_size_t i = 0; i < K; ++i) {
        mp_size_t coeflen;
        pfcb[i] = (mp_ptr)(pfcb + K) + i * nlen;
        if (nbrest > 0) {
            coeflen = M + (i == K - 1);
            coeflen = LMMP_MIN(nbrest, coeflen);
            nbrest -= coeflen;
            lmmp_fft_extract_coef_(pfcb[i], numb, M * i, coeflen, msr.lenw);
            if (i > 0)
                lmmp_fft_shl_coef_(&msr, pfcb + i, i * n >> k);
        } else {
            lmmp_zero(pfcb[i], nlen);
        }
    }
    lmmp_fft_(&msr, pfcb, k, n >> (k - 1));

    lmmp_mul_fermat_recurse_(&msr, pfca, pfcb, K);

    lmmp_ifft_(&msr, pfca, k, n >> (k - 1));

    lmmp_mul_fermat_recombine_(&msr, dst, pfca, K, k, n, M, rn);

    // 处理模 B^rn+1 的溢出
    if (dst[rn] && !lmmp_zero_q_(dst, rn)) {
        dst[rn] = 0;
        lmmp_dec(dst);
    }

    lmmp_fft_memstack_(&msr, 0);
}

void lmmp_mul_mersenne_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb) {
    const int nsqr = 1;
    mp_size_t N = rn * LIMB_BITS;         // 结果总比特数
    mp_size_t k = lmmp_fft_best_k_(rn);   // 最优FFT层数
    mp_size_t K = ((mp_size_t)1) << k;    // FFT块数（2^k）
    // 断言：N必须是K的整数倍
    lmmp_debug_assert(!(N & (K - 1)));
    mp_size_t M = N >> k;     // 每个块的比特数
    mp_size_t n = 2 * M + k;  // 扩展系数长度（梅森数比费马数少2）

    // 规整n：必须是LIMB_BITS和K/2的整数倍
    n = (n + LIMB_BITS - 1) & (-LIMB_BITS);
    n = (((n - 1) >> (k - 1)) + 1) << (k - 1);

    // 初始化内存栈
    fft_memstack msr;
    msr.maxdepth = -1;
    msr.tempdepth = -1;
    msr.lenw = n / LIMB_BITS;
    mp_size_t nlen = msr.lenw + 1;

    msr.temp_coef = (mp_ptr)lmmp_fft_memstack_(&msr, (((nlen + 1) << (k + nsqr)) + nlen) * LIMB_BYTES);

    mp_ptr *pfca = (mp_ptr*)(msr.temp_coef + nlen), *pfcb = pfca;
    mp_size_t narest = na * LIMB_BITS, nbrest = nb * LIMB_BITS;

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

    pfcb += (nlen + 1) << k;
    for (mp_size_t i = 0; i < K; ++i) {
        mp_size_t coeflen;
        pfcb[i] = (mp_ptr)(pfcb + K) + i * nlen;
        if (nbrest > 0) {
            coeflen = LMMP_MIN(nbrest, M);
            nbrest -= coeflen;
            lmmp_fft_extract_coef_(pfcb[i], numb, M * i, coeflen, msr.lenw);
        } else {
            lmmp_zero(pfcb[i], nlen);
        }
    }
    lmmp_fft_(&msr, pfcb, k, n >> (k - 1));

    lmmp_mul_fermat_recurse_(&msr, pfca, pfcb, K);

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

void lmmp_mul_fermat_cache_init_(
    mp_ptr        dst,
    mp_size_t      rn,
    mp_srcptr    numa,
    mp_size_t      na,
    mp_srcptr    numb,
    mp_size_t      nb,
    fft_gr_cache* ctx
) {
    lmmp_param_assert(na > 0 && nb > 0);
    lmmp_param_assert(dst != NULL && numa != NULL && numb != NULL);
    lmmp_param_assert(ctx != NULL);
    ctx->na = na;
    ctx->rn = rn;
    ctx->N = rn * LIMB_BITS;
    ctx->k = lmmp_fft_best_k_(rn);
    ctx->K = ((mp_size_t)1) << ctx->k;
    lmmp_debug_assert(!(ctx->N & (ctx->K - 1)));
    ctx->M = ctx->N >> ctx->k;
    ctx->n = 2 * ctx->M + ctx->k + 2;
    ctx->n = (ctx->n + LIMB_BITS - 1) & (-LIMB_BITS);
    ctx->n = (((ctx->n - 1) >> ctx->k) + 1) << ctx->k;

    fft_memstack* bmsr = &ctx->bmsr;
    fft_memstack* amsr = &ctx->amsr;
    amsr->maxdepth = -1;
    amsr->tempdepth = -1;
    amsr->lenw = ctx->n / LIMB_BITS;
    ctx->nlen = amsr->lenw + 1;
    ctx->a_size = (((ctx->nlen + 1) << (ctx->k)) + ctx->nlen) * LIMB_BYTES;
    amsr->temp_coef = (mp_ptr)lmmp_fft_memstack_(amsr, ctx->a_size);

    mp_ptr* pfca = (mp_ptr*)(amsr->temp_coef + ctx->nlen);
    mp_ptr* pfcb = NULL;

    bmsr->maxdepth = -1;
    bmsr->tempdepth = -1;
    bmsr->lenw = ctx->n / LIMB_BITS;
    bmsr->temp_coef = (mp_ptr)lmmp_fft_memstack_(bmsr, ctx->a_size);
    ctx->temp_coefb = bmsr->temp_coef;
    pfcb = (mp_ptr*)(bmsr->temp_coef + ctx->nlen);

    mp_size_t narest = na * LIMB_BITS, nbrest = nb * LIMB_BITS;
    for (mp_size_t i = 0; i < ctx->K; ++i) {
        mp_size_t coeflen;
        pfca[i] = (mp_ptr)(pfca + ctx->K) + i * ctx->nlen;
        if (narest > 0) {
            coeflen = ctx->M + (i == ctx->K - 1);
            coeflen = LMMP_MIN(narest, coeflen);
            narest -= coeflen;
            lmmp_fft_extract_coef_(pfca[i], numa, ctx->M * i, coeflen, amsr->lenw);
            if (i > 0)
                lmmp_fft_shl_coef_(amsr, pfca + i, i * ctx->n >> ctx->k);
        } else {
            lmmp_zero(pfca[i], ctx->nlen);
        }
    }
    lmmp_fft_(amsr, pfca, ctx->k, ctx->n >> (ctx->k - 1));

    for (mp_size_t i = 0; i < ctx->K; ++i) {
        mp_size_t coeflen;
        pfcb[i] = (mp_ptr)(pfcb + ctx->K) + i * ctx->nlen;
        if (nbrest > 0) {
            coeflen = ctx->M + (i == ctx->K - 1);
            coeflen = LMMP_MIN(nbrest, coeflen);
            nbrest -= coeflen;
            lmmp_fft_extract_coef_(pfcb[i], numb, ctx->M * i, coeflen, bmsr->lenw);
            if (i > 0)
                lmmp_fft_shl_coef_(bmsr, pfcb + i, i * ctx->n >> ctx->k);
        } else {
            lmmp_zero(pfcb[i], ctx->nlen);
        }
    }
    lmmp_fft_(bmsr, pfcb, ctx->k, ctx->n >> (ctx->k - 1));

    lmmp_mul_fermat_recurse_(amsr, pfca, pfcb, ctx->K);

    lmmp_ifft_(amsr, pfca, ctx->k, ctx->n >> (ctx->k - 1));

    lmmp_mul_fermat_recombine_(amsr, dst, pfca, ctx->K, ctx->k, ctx->n, ctx->M, ctx->rn);

    if (dst[rn] && !lmmp_zero_q_(dst, rn)) {
        dst[rn] = 0;
        lmmp_dec(dst);
    }
    lmmp_fft_memstack_(amsr, 0);
}

void lmmp_mul_fermat_cache_(mp_ptr dst, mp_srcptr numa, fft_gr_cache* ctx) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(ctx != NULL);
    fft_memstack* bmsr = &ctx->bmsr;
    fft_memstack* amsr = &ctx->amsr;

    amsr->maxdepth = -1;
    amsr->tempdepth = -1;
    amsr->temp_coef = (mp_ptr)lmmp_fft_memstack_(amsr, ctx->a_size);

    bmsr->temp_coef = ctx->temp_coefb;
    mp_ptr* pfca = (mp_ptr*)(amsr->temp_coef + ctx->nlen);
    mp_ptr* pfcb = (mp_ptr*)(bmsr->temp_coef + ctx->nlen);

    mp_size_t narest = ctx->na * LIMB_BITS;

    for (mp_size_t i = 0; i < ctx->K; ++i) {
        mp_size_t coeflen;
        pfca[i] = (mp_ptr)(pfca + ctx->K) + i * ctx->nlen;
        if (narest > 0) {
            coeflen = ctx->M + (i == ctx->K - 1);
            coeflen = LMMP_MIN(narest, coeflen);
            narest -= coeflen;
            lmmp_fft_extract_coef_(pfca[i], numa, ctx->M * i, coeflen, amsr->lenw);
            if (i > 0)
                lmmp_fft_shl_coef_(amsr, pfca + i, i * ctx->n >> ctx->k);
        } else {
            lmmp_zero(pfca[i], ctx->nlen);
        }
    }
    lmmp_fft_(amsr, pfca, ctx->k, ctx->n >> (ctx->k - 1));

    lmmp_mul_fermat_recurse_(amsr, pfca, pfcb, ctx->K);

    lmmp_ifft_(amsr, pfca, ctx->k, ctx->n >> (ctx->k - 1));

    lmmp_mul_fermat_recombine_(amsr, dst, pfca, ctx->K, ctx->k, ctx->n, ctx->M, ctx->rn);

    if (dst[ctx->rn] && !lmmp_zero_q_(dst, ctx->rn)) {
        dst[ctx->rn] = 0;
        lmmp_dec(dst);
    }
    lmmp_fft_memstack_(amsr, 0);
}

void lmmp_mul_mersenne_cache_init_(
    mp_ptr        dst,
    mp_size_t      rn,
    mp_srcptr    numa,
    mp_size_t      na,
    mp_srcptr    numb,
    mp_size_t      nb,
    fft_gr_cache* ctx
) {
    lmmp_param_assert(na > 0 && nb > 0);
    lmmp_param_assert(dst != NULL && numa != NULL && numb != NULL);
    lmmp_param_assert(ctx != NULL);
    ctx->na = na;
    ctx->rn = rn;
    ctx->N = rn * LIMB_BITS;
    ctx->k = lmmp_fft_best_k_(rn);
    ctx->K = ((mp_size_t)1) << ctx->k;
    lmmp_debug_assert(!(ctx->N & (ctx->K - 1)));
    ctx->M = ctx->N >> ctx->k;
    ctx->n = 2 * ctx->M + ctx->k;
    ctx->n = (ctx->n + LIMB_BITS - 1) & (-LIMB_BITS);
    ctx->n = (((ctx->n - 1) >> (ctx->k - 1)) + 1) << (ctx->k - 1);

    fft_memstack* bmsr = &ctx->bmsr;
    fft_memstack* amsr = &ctx->amsr;
    amsr->maxdepth = -1;
    amsr->tempdepth = -1;
    amsr->lenw = ctx->n / LIMB_BITS;
    ctx->nlen = amsr->lenw + 1;
    ctx->a_size = (((ctx->nlen + 1) << (ctx->k)) + ctx->nlen) * LIMB_BYTES;
    amsr->temp_coef = (mp_ptr)lmmp_fft_memstack_(amsr, ctx->a_size);
    mp_ptr* pfca = (mp_ptr*)(amsr->temp_coef + ctx->nlen);
    mp_ptr* pfcb = NULL;

    bmsr->maxdepth = -1;
    bmsr->tempdepth = -1;
    bmsr->lenw = ctx->n / LIMB_BITS;
    bmsr->temp_coef = (mp_ptr)lmmp_fft_memstack_(bmsr, ctx->a_size);
    ctx->temp_coefb = bmsr->temp_coef;
    pfcb = (mp_ptr*)(bmsr->temp_coef + ctx->nlen);

    mp_size_t narest = na * LIMB_BITS, nbrest = nb * LIMB_BITS;

    for (mp_size_t i = 0; i < ctx->K; ++i) {
        mp_size_t coeflen;
        pfca[i] = (mp_ptr)(pfca + ctx->K) + i * ctx->nlen;
        if (narest > 0) {
            coeflen = ctx->M + (i == ctx->K - 1);
            coeflen = LMMP_MIN(narest, ctx->M);
            narest -= coeflen;
            lmmp_fft_extract_coef_(pfca[i], numa, ctx->M * i, coeflen, amsr->lenw);
        } else {
            lmmp_zero(pfca[i], ctx->nlen);
        }
    }
    lmmp_fft_(amsr, pfca, ctx->k, ctx->n >> (ctx->k - 1));

    for (mp_size_t i = 0; i < ctx->K; ++i) {
        mp_size_t coeflen;
        pfcb[i] = (mp_ptr)(pfcb + ctx->K) + i * ctx->nlen;
        if (nbrest > 0) {
            coeflen = LMMP_MIN(nbrest, ctx->M);
            nbrest -= coeflen;
            lmmp_fft_extract_coef_(pfcb[i], numb, ctx->M * i, coeflen, bmsr->lenw);
        } else {
            lmmp_zero(pfcb[i], ctx->nlen);
        }
    }
    lmmp_fft_(bmsr, pfcb, ctx->k, ctx->n >> (ctx->k - 1));

    lmmp_mul_fermat_recurse_(amsr, pfca, pfcb, ctx->K);

    lmmp_ifft_(amsr, pfca, ctx->k, ctx->n >> (ctx->k - 1));

    mp_size_t rhead = 0, maxc = 0;
    for (mp_size_t i = 0; i < ctx->K; ++i) {
        lmmp_fft_shr_coef_(amsr, pfca + i, ctx->k);
        mp_ptr nums = pfca[i];

        if (nums[ctx->nlen - 1]) {
            lmmp_dec(nums);
            lmmp_debug_assert(nums[ctx->nlen - 1] == 1);
            nums[ctx->nlen - 1] = 0;
        }

        mp_size_t roffset = i * ctx->M;
        mp_size_t shl = roffset & (LIMB_BITS - 1);
        roffset /= LIMB_BITS;

        if (shl)
            lmmp_shl_(nums, nums, ctx->nlen, shl);

        if (i == 0) {
            lmmp_copy(dst, nums, ctx->nlen);
            rhead = ctx->nlen;
        } else if (roffset + ctx->nlen <= rn) {
            lmmp_add_(dst + roffset, nums, ctx->nlen, dst + roffset, rhead - roffset);
            rhead = roffset + ctx->nlen;
        } else {
            maxc += lmmp_add_(dst + roffset, nums, rn - roffset, dst + roffset, rhead - roffset);
            maxc += lmmp_add_(dst, dst, rn, nums + rn - roffset, ctx->nlen + roffset - rn);
            rhead = rn;
        }
    }

    if (!lmmp_add_1_(dst, dst, rn, 1 + maxc))
        lmmp_dec(dst);

    lmmp_fft_memstack_(amsr, 0);
}

void lmmp_mul_mersenne_cache_(mp_ptr dst, mp_srcptr numa, fft_gr_cache* ctx) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(ctx != NULL);
    fft_memstack* bmsr = &ctx->bmsr;
    fft_memstack* amsr = &ctx->amsr;

    amsr->maxdepth = -1;
    amsr->tempdepth = -1;
    amsr->lenw = ctx->n / LIMB_BITS;
    amsr->temp_coef = (mp_ptr)lmmp_fft_memstack_(amsr, ctx->a_size);

    bmsr->temp_coef = ctx->temp_coefb;
    mp_ptr* pfca = (mp_ptr*)(amsr->temp_coef + ctx->nlen);
    mp_ptr* pfcb = (mp_ptr*)(bmsr->temp_coef + ctx->nlen);

    mp_size_t narest = ctx->na * LIMB_BITS;

    for (mp_size_t i = 0; i < ctx->K; ++i) {
        mp_size_t coeflen;
        pfca[i] = (mp_ptr)(pfca + ctx->K) + i * ctx->nlen;
        if (narest > 0) {
            coeflen = ctx->M + (i == ctx->K - 1);
            coeflen = LMMP_MIN(narest, ctx->M);
            narest -= coeflen;
            lmmp_fft_extract_coef_(pfca[i], numa, ctx->M * i, coeflen, amsr->lenw);
        } else {
            lmmp_zero(pfca[i], ctx->nlen);
        }
    }
    lmmp_fft_(amsr, pfca, ctx->k, ctx->n >> (ctx->k - 1));

    lmmp_mul_fermat_recurse_(amsr, pfca, pfcb, ctx->K);

    lmmp_ifft_(amsr, pfca, ctx->k, ctx->n >> (ctx->k - 1));

    mp_size_t rhead = 0, maxc = 0;
    for (mp_size_t i = 0; i < ctx->K; ++i) {
        lmmp_fft_shr_coef_(amsr, pfca + i, ctx->k);
        mp_ptr nums = pfca[i];

        if (nums[ctx->nlen - 1]) {
            lmmp_dec(nums);
            lmmp_debug_assert(nums[ctx->nlen - 1] == 1);
            nums[ctx->nlen - 1] = 0;
        }

        mp_size_t roffset = i * ctx->M;
        mp_size_t shl = roffset & (LIMB_BITS - 1);
        roffset /= LIMB_BITS;

        if (shl)
            lmmp_shl_(nums, nums, ctx->nlen, shl);

        if (i == 0) {
            lmmp_copy(dst, nums, ctx->nlen);
            rhead = ctx->nlen;
        } else if (roffset + ctx->nlen <= ctx->rn) {
            lmmp_add_(dst + roffset, nums, ctx->nlen, dst + roffset, rhead - roffset);
            rhead = roffset + ctx->nlen;
        } else {
            maxc += lmmp_add_(dst + roffset, nums, ctx->rn - roffset, dst + roffset, rhead - roffset);
            maxc += lmmp_add_(dst, dst, ctx->rn, nums + ctx->rn - roffset, ctx->nlen + roffset - ctx->rn);
            rhead = ctx->rn;
        }
    }

    if (!lmmp_add_1_(dst, dst, ctx->rn, 1 + maxc))
        lmmp_dec(dst);

    lmmp_fft_memstack_(amsr, 0);
}

void lmmp_mul_fft_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb) {
    lmmp_param_assert(na > 0 && nb > 0);
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(dst != NULL && numa != NULL && numb != NULL);
    mp_size_t hn = lmmp_fft_next_size_((na + nb + 1) >> 1);
    lmmp_assert(na + nb > hn);
    mp_ptr tp = ALLOC_TYPE(hn + 1, mp_limb_t);

    mp_srcptr amodm = numa;
    mp_size_t nam = na;
    if (na > hn) {
        /*
          Z = B^hb - 1
          amodm = a mod Z
         */
        if (lmmp_add_(dst, numa, hn, numa + hn, na - hn))
            lmmp_inc(dst);
        amodm = dst;
        nam = hn;
    }
    lmmp_mul_mersenne_(dst, hn, amodm, nam, numb, nb);

    mp_srcptr amodp = numa;
    mp_size_t nap = na;
    if (na > hn) {
        /*
          Z = B^hp + 1
          amodp = a mod Z
         */
        tp[hn] = 0;
        if (lmmp_sub_(tp, numa, hn, numa + hn, na - hn))
            lmmp_inc(tp);
        amodp = tp;
        nap = hn + 1;
    }
    lmmp_mul_fermat_(tp, hn, amodp, nap, numb, nb);

    mp_limb_t cy = lmmp_shr1add_nc_(dst, dst, tp, hn, tp[hn]);
    cy <<= LIMB_BITS - 1;
    dst[hn - 1] += cy;
    if (dst[hn - 1] < cy)
        lmmp_inc(dst);

    if (na + nb == 2 * hn) {
        cy = tp[hn] + lmmp_sub_n_(dst + hn, dst, tp, hn);
        // cy==1 means [tp,hn+1]!=0, then [dst,hn]!=0
        // cy==2 is impossible since [tp,hn+1] is normalized.
        // so the following dec won't overflow.
        lmmp_dec_1(dst, cy);
    } else {
        cy = lmmp_sub_n_(dst + hn, dst, tp, na + nb - hn);
        cy = tp[hn] + lmmp_sub_nc_(tp + na + nb - hn, dst + na + nb - hn, tp + na + nb - hn, 2 * hn - (na + nb), cy);
        cy = lmmp_sub_1_(dst, dst, na + nb, cy);
    }
    lmmp_free(tp);
}

void lmmp_mul_fft_cache_init_(
    mp_ptr     dst,
    mp_size_t   hn,
    mp_srcptr numa,
    mp_size_t   na,
    mp_srcptr numb,
    mp_size_t   nb,
    fft_cache* ctx
) {
    lmmp_param_assert(na > 0 && nb > 0);
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(na + nb > hn);
    lmmp_param_assert(dst != NULL && numa != NULL && numb != NULL);
    lmmp_param_assert(ctx != NULL);
    ctx->hn = hn;
    ctx->na = na;
    ctx->nb = nb;
    ctx->tp = ALLOC_TYPE(hn + 1, mp_limb_t);

    mp_srcptr amodm = numa;
    mp_size_t nam = na;
    if (na > hn) {
        if (lmmp_add_(dst, numa, hn, numa + hn, na - hn))
            lmmp_inc(dst);
        amodm = dst;
        nam = hn;
    }
    lmmp_mul_mersenne_cache_init_(dst, hn, amodm, nam, numb, nb, &ctx->mersenne);

    mp_srcptr amodp = numa;
    mp_size_t nap = na;
    if (na > hn) {
        ctx->tp[hn] = 0;
        if (lmmp_sub_(ctx->tp, numa, hn, numa + hn, na - hn))
            lmmp_inc(ctx->tp);
        amodp = ctx->tp;
        nap = hn + 1;
    }
    lmmp_mul_fermat_cache_init_(ctx->tp, hn, amodp, nap, numb, nb, &ctx->fermat);

    mp_limb_t cy = lmmp_shr1add_nc_(dst, dst, ctx->tp, hn, ctx->tp[hn]);
    cy <<= LIMB_BITS - 1;
    dst[hn - 1] += cy;
    if (dst[hn - 1] < cy)
        lmmp_inc(dst);

    if (na + nb == 2 * hn) {
        cy = ctx->tp[hn] + lmmp_sub_n_(dst + hn, dst, ctx->tp, hn);
        // cy==1 means [tp,hn+1]!=0, then [dst,hn]!=0
        // cy==2 is impossible since [tp,hn+1] is normalized.
        // so the following dec won't overflow.
        lmmp_dec_1(dst, cy);
    } else {
        cy = lmmp_sub_n_(dst + hn, dst, ctx->tp, na + nb - hn);
        cy = ctx->tp[hn] + lmmp_sub_nc_(ctx->tp + na + nb - hn, dst + na + nb - hn, ctx->tp + na + nb - hn, 2 * hn - (na + nb), cy);
        cy = lmmp_sub_1_(dst, dst, na + nb, cy);
    }
}

void lmmp_mul_fft_cache_(mp_ptr dst, mp_srcptr numa, fft_cache* ctx) {
    lmmp_param_assert(dst != NULL && numa != NULL);
    lmmp_param_assert(ctx != NULL);
    mp_srcptr amodm = numa;
    if (ctx->na > ctx->hn) {
        if (lmmp_add_(dst, numa, ctx->hn, numa + ctx->hn, ctx->na - ctx->hn))
            lmmp_inc(dst);
        amodm = dst;
    }
    lmmp_mul_mersenne_cache_(dst, amodm, &ctx->mersenne);

    mp_srcptr amodp = numa;
    if (ctx->na > ctx->hn) {
        ctx->tp[ctx->hn] = 0;
        if (lmmp_sub_(ctx->tp, numa, ctx->hn, numa + ctx->hn, ctx->na - ctx->hn))
            lmmp_inc(ctx->tp);
        amodp = ctx->tp;
    }
    lmmp_mul_fermat_cache_(ctx->tp, amodp, &ctx->fermat);

    mp_limb_t cy = lmmp_shr1add_nc_(dst, dst, ctx->tp, ctx->hn, ctx->tp[ctx->hn]);
    cy <<= LIMB_BITS - 1;
    dst[ctx->hn - 1] += cy;
    if (dst[ctx->hn - 1] < cy)
        lmmp_inc(dst);

    if (ctx->na + ctx->nb == 2 * ctx->hn) {
        cy = ctx->tp[ctx->hn] + lmmp_sub_n_(dst + ctx->hn, dst, ctx->tp, ctx->hn);
        lmmp_dec_1(dst, cy);
    } else {
        cy = lmmp_sub_n_(dst + ctx->hn, dst, ctx->tp, ctx->na + ctx->nb - ctx->hn);
        cy = ctx->tp[ctx->hn] + lmmp_sub_nc_(ctx->tp + ctx->na + ctx->nb - ctx->hn, dst + ctx->na + ctx->nb - ctx->hn,
                                             ctx->tp + ctx->na + ctx->nb - ctx->hn, 2 * ctx->hn - (ctx->na + ctx->nb),
                                             cy);
        cy = lmmp_sub_1_(dst, dst, ctx->na + ctx->nb, cy);
    }
}

void lmmp_mul_fft_unbalance_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict numa,
    mp_size_t            na,
    mp_srcptr restrict numb,
    mp_size_t            nb
) {
    lmmp_param_assert(na >= 3 * nb);
    mp_ptr restrict ws = ALLOC_TYPE(nb, mp_limb_t);
    mp_size_t sna = 3 * nb;
    mp_size_t hn = lmmp_fft_next_size_((sna + nb + 1) >> 1);
    sna = (hn << 1) - 1 - nb;
    fft_cache ctx;
    lmmp_mul_fft_cache_init_(dst, hn, numa, sna, numb, nb, &ctx);
    dst += sna;
    numa += sna;
    na -= sna;
    lmmp_copy(ws, dst, nb);
    while (na >= sna) {
        lmmp_mul_fft_cache_(dst, numa, &ctx);
        if (lmmp_add_n_(dst, dst, ws, nb))
            lmmp_inc(dst + nb);
        dst += sna;
        numa += sna;
        na -= sna;
        lmmp_copy(ws, dst, nb);
    }
    lmmp_fft_cache_free_(&ctx);
    // remaining na < sna
    if (na >= nb)
        lmmp_mul_(dst, numa, na, numb, nb);
    else if (na > 0)
        lmmp_mul_(dst, numb, nb, numa, na);
    else  // na == 0
        lmmp_zero(dst, nb);
    if (lmmp_add_n_(dst, dst, ws, nb))
        lmmp_inc(dst + nb);
    lmmp_free(ws);
}

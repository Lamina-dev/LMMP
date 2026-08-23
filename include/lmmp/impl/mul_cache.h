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

#ifndef __LMMP_MUL_CACHE_H__
#define __LMMP_MUL_CACHE_H__

#include "fft_ssa.h"


typedef struct {
    mp_size_t a_size;
    mp_ptr temp_coefb;
    mp_size_t na;
    mp_size_t rn;
    mp_size_t N;
    mp_size_t k;
    mp_size_t K;
    mp_size_t M;
    mp_size_t n;
    mp_size_t nlen;
    fft_memstack amsr;
    fft_memstack bmsr;
} fft_gr_cache;

typedef struct {
    fft_gr_cache fermat;
    fft_gr_cache mersenne;
    mp_ptr tp;
    mp_size_t hn;
    mp_size_t na;
    mp_size_t nb;
} fft_cache;

/**
 * @brief 释放费马或梅森数模乘法缓存上下文
 * @param ctx 费马或梅森数模乘法缓存上下文
 */
static inline void lmmp_fft_gr_cache_free_(fft_gr_cache* ctx) {
    lmmp_fft_memstack_(&ctx->bmsr, 0);
}

/**
 * @brief 释放FFT模乘法缓存上下文
 * @param ctx FFT模乘法缓存上下文
 */
static inline void lmmp_fft_cache_free_(fft_cache* ctx) {
    lmmp_fft_gr_cache_free_(&ctx->fermat);
    lmmp_fft_gr_cache_free_(&ctx->mersenne);
    lmmp_free(ctx->tp);
}

/**
 * @brief 费马数模乘法（缓存版） [dst,rn+1]=[numa,na]*[numb,nb] mod B^rn+1
 *        第二个操作数将被缓存
 * @param dst 输出结果缓冲区，长度至少为 rn+1
 * @param rn 模运算的阶数参数，rn = lmmp_fft_next_size_((na + nb + 1) >> 1)
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @param ctx 费马数模乘法缓存上下文
 * @warning eqsep(dst,[numa|numb]), 0<=[numa,na]<2*B^rn, 0<=[numb,nb]<2*B^rn, rn = lmmp_fft_next_size_((na+nb+1)>>1)
 * @note [numb,nb]将会被缓存，第二个乘数始终保持不变时，在后续计算中可以调用lmmp_mul_fermat_cache_()函数节省40%的计算时间
 *       numb可以在lmmp_mul_fermat_cache_()函数前被释放，不会影响后续计算
 * @return 无返回值，结果存储在dst中
 */
void lmmp_mul_fermat_cache_init_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb,
                                 fft_gr_cache* ctx);

/**
 * @brief 费马数模乘法（缓存版） [dst,rn+1]=[numa,na]*[numb,nb] mod B^rn+1
 * @param dst 输出结果缓冲区，长度为 lmmp_mul_fermat_cache_init_()函数中输入参数的 rn+1
 * @param numa 第一个输入操作数，长度为 lmmp_mul_fermat_cache_init_()函数中输入参数的 na
 * @param ctx 费马数模乘法缓存上下文
 * @warning eqsep(dst,numa), 0<=[numa,na]<2*B^rn
 * @note 使用被缓存的[numb,nb]作为第二个乘数，numb可以在lmmp_mul_fermat_cache_()函数前被释放，不会影响后续计算
 * @return 无返回值，结果存储在dst中
 */
void lmmp_mul_fermat_cache_(mp_ptr dst, mp_srcptr numa, fft_gr_cache* ctx);

/**
 * @brief 梅森数模乘法 [dst,rn] = [numa,na]*[numb,nb] mod B^rn-1
 *        第二个操作数将被缓存
 * @param dst 输出结果缓冲区，长度至少为 rn
 * @param rn 模运算的阶数参数，rn = lmmp_fft_next_size_((na + nb + 1) >> 1)
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @param ctx 梅森数模乘法缓存上下文
 * @warning eqsep(dst,[numa|numb]), 0<=[numa,na]<B^rn, 0<=[numb,nb]<B^rn, rn = lmmp_fft_next_size_((na+nb+1)>>1)
 * @note [numb,nb]将会被缓存，第二个乘数始终保持不变时，在后续计算中可以调用lmmp_mul_mersenne_cache_()函数节省40%的计算时间
 *       numb可以在lmmp_mul_mersenne_cache_()函数前被释放，不会影响后续计算
 * @return 无返回值，结果存储在dst中
 */
void lmmp_mul_mersenne_cache_init_(mp_ptr dst, mp_size_t rn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb,
                                   fft_gr_cache* ctx);

/**
 * @brief 梅森数模乘法 [dst,rn] = [numa,na]*[numb,nb] mod B^rn-1
 * @param dst 输出结果缓冲区，长度为 lmmp_mul_mersenne_cache_init_()函数中输入参数的 rn
 * @param numa 第一个输入操作数，长度为 lmmp_mul_mersenne_cache_init_()函数中输入参数的 na
 * @param ctx 梅森数模乘法缓存上下文
 * @warning eqsep(dst,numa), 0<=[numa,na]<B^rn
 * @note 使用被缓存的[numb,nb]作为第二个乘数，numb可以在lmmp_mul_mersenne_cache_()函数前被释放，不会影响后续计算
 * @return 无返回值，结果存储在dst中
 */
void lmmp_mul_mersenne_cache_(mp_ptr dst, mp_srcptr numa, fft_gr_cache* ctx);

/**
 * @brief FFT乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 *        第二个操作数将被缓存
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param hn 模运算的阶数参数，hn = lmmp_fft_next_size_((na + nb + 1) >> 1)
 * @param numa 第一个输入操作数，长度为 na
 * @param na 第一个操作数的 limb 长度
 * @param numb 第二个输入操作数，长度为 nb
 * @param nb 第二个操作数的 limb 长度
 * @param ctx FFT乘法缓存上下文
 * @warning ???<=nb<=na, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL, ctx!=NULL
 * @note [numb,nb]将会被缓存，第二个乘数始终保持不变时，在后续计算中可以调用lmmp_mul_fft_cache_()函数节省40%的计算时间
 *       numb可以在lmmp_mul_fft_cache_()函数前被释放，不会影响后续计算
 * @return 无返回值，结果存储在dst中
 */
void lmmp_mul_fft_cache_init_(mp_ptr dst, mp_size_t hn, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb,
                              fft_cache* ctx);

/**
 * @brief FFT乘法运算 [dst,na+nb] = [numa,na] * [numb,nb]
 * @param dst 输出结果缓冲区，长度至少为 na+nb
 * @param numa 第一个输入操作数，长度为 na
 * @param ctx FFT乘法缓存上下文
 * @warning ???<=na, sep(dst,numa), dst!=NULL, numa!=NULL, numb!=NULL, ctx!=NULL
 * @note [numb,nb]将会被缓存，第二个乘数始终保持不变时，在后续计算中可以调用lmmp_mul_fft_cache_()函数节省40%的计算时间
 *       numb可以在lmmp_mul_fft_cache_()函数前被释放，不会影响后续计算
 * @return 无返回值，结果存储在dst中
 */
void lmmp_mul_fft_cache_(mp_ptr dst, mp_srcptr numa, fft_cache* ctx);


#endif // __LMMP_MUL_CACHE_H__
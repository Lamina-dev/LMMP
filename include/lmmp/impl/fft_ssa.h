/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#ifndef __LMMP_FFT_SSA_H__
#define __LMMP_FFT_SSA_H__

#include "../lmmpn.h"

#define FFT_MEMSTACK_DEPTH 16

typedef struct {
    mp_ptr temp_coef;       // 用于数据交换的临时系数数组
    mp_size_t lenw;         // 系数的机器字（limb）长度
    mp_ssize_t maxdepth;    // 内存栈的最大深度（已分配的层数）
    mp_ssize_t tempdepth;   // 内存栈的当前深度（正在使用的层数）
    void* mem[FFT_MEMSTACK_DEPTH];          // 存储内存块的指针
    mp_size_t memsize[FFT_MEMSTACK_DEPTH];  // 存储每层内存块的大小（以字节为单位）
} fft_memstack;

/**
 * @brief 查找对于 m>=n 的模 B^m+1 FFT运算的最优k值
 * @param n - 输入的机器字长度
 * @return 最优的k值
 */
mp_size_t lmmp_fft_best_k_(mp_size_t n);

/**
 * @brief FFT内存栈的分配/释放接口
 * @param ms 内存栈结构体栈帧
 * @param size 分配大小（字节），size=0表示释放当前层内存
 * @return 分配成功：返回mp_ptr*；释放：返回0
 */
void* lmmp_fft_memstack_(fft_memstack* ms, mp_size_t size);

/**
 * @brief [dst,lenw+1] = [(bit*)numa+bitoffset, bits]
 * @param dst 输出系数数组（长度lenw+1）
 * @param numa 输入大数指针
 * @param bitoffset 起始比特偏移量（>=0）
 * @param bits 提取的比特数（0 < bits <= LIMB_BITS*lenw）
 * @param lenw 输出系数的机器字长度
 * @warning bitoffset>=0, 0<bits<=LIMB_BITS*lenw, sep(dst,numa)
 */
void lmmp_fft_extract_coef_(mp_ptr dst, mp_srcptr numa, mp_size_t bitoffset, mp_size_t bits, mp_size_t lenw);

/**
 * @brief 对模 2^n+1 的系数执行左移操作
 * @param ms 内存栈结构体指针
 * @param coef 输入输出系数数组指针（指针的指针，用于交换内存）
 * @param shl 左移的比特数（0<shl<2*n）
 * @warning n = ms->lenw * LIMB_BITS
 *         *coef 已伪归一化（mod 2^n+1）
 *         ms->temp_coef 至少有 lenw+1 个机器字
 */
void lmmp_fft_shl_coef_(fft_memstack* ms, mp_ptr* coef, mp_size_t shl);

/**
 * @brief 对模 2^n+1 的系数执行右移操作
 * 右移shr位 = 左移(2n - shr)位（mod 2^n+1的循环特性）
 * @param ms 内存栈结构体指针
 * @param coef 输入输出系数数组指针
 * @param shr 右移的比特数（0 < shr < 2*n）
 */
void lmmp_fft_shr_coef_(fft_memstack* ms, mp_ptr* coef, mp_size_t shr);

/**
 * @brief FFT蝶形运算（Butterfly Operation）
 * (a,b) = (a + b, (a-b) << w ) mod 2^n+1
 * a=[coef[0],ms->lenw+1], b=[coef[wing],ms->lenw+1], n=ms->lenw * LIMB_BITS
 * @param ms 内存栈结构体指针
 * @param coef 系数数组指针数组（coef[0]=a, coef[wing]=b）
 * @param wing b的索引
 * @param w 左移的比特数（0<=w<n）
 * @warning n = ms->lenw * LIMB_BITS
 *          a,b 均已伪归一化（mod 2^n+1）
 *          ms->temp_coef 有至少 lenw + 1 个字长
 */
void lmmp_fft_bfy_(fft_memstack* ms, mp_ptr* coef, mp_size_t wing, mp_size_t w);

/**
 * @brief FFT蝶形运算（Butterfly Operation）
 * (a,b) = (a+(b>>w), a-(b>>w)) mod 2^n+1
 * a=[coef[0],ms->lenw+1], b=[coef[wing],ms->lenw+1], n=ms->lenw * LIMB_BITS
 * @param ms 内存栈结构体指针
 * @param coef 系数数组指针数组（coef[0]=a, coef[wing]=b）
 * @param wing b的索引
 * @param w 左移的比特数（0<=w<n）
 * @warning n = ms->lenw * LIMB_BITS
 *          a,b 均已伪归一化（mod 2^n+1）
 *          ms->temp_coef 有至少 lenw + 1 个字长
 */
void lmmp_ifft_bfy_(fft_memstack* ms, mp_ptr* coef, mp_size_t wing, mp_size_t w);

void lmmp_fft_(fft_memstack* ms, mp_ptr* coef, mp_size_t k, mp_size_t w);

void lmmp_ifft_(fft_memstack* ms, mp_ptr* coef, mp_size_t k, mp_size_t w);

/**
 * @brief 费马变换 模 B^n+1 乘法的结果合并
 * @param ms 内存栈结构体指针
 * @param dst 输出结果数组
 * @param pfca FFT系数数组指针数组
 * @param K FFT块数（2^k）
 * @param k FFT层数
 * @param n 系数总比特数
 * @param M 每个块的比特数
 * @param rn 结果长度（机器字）
 */
void lmmp_mul_fermat_recombine_(fft_memstack* ms, mp_ptr dst, mp_ptr* pfca, mp_size_t K, mp_size_t k, mp_size_t n,
                                mp_size_t M, mp_size_t rn);

#endif // __LMMP_FFT_SSA_H__
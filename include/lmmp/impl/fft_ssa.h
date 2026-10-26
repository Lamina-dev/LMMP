/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __LMMP_FFT_SSA_H__
#define __LMMP_FFT_SSA_H__

#include "../lmmpn.h"

#define FFT_MEMSTACK_DEPTH 16

/* 调优模式下的 FFT 表访问接口（实现在 src/lmmp/lmmpn/fft_ssa.c） */
#ifdef LMMP_TUNE
/**
 * @brief 安装候选 FFT 表（含哨兵行自动补齐）
 * @param rows 平铺的 [阈值,k] 数组（不含哨兵行）
 * @param count 行数（不含哨兵）
 * @warning rows 须满足：阈值严格递增；k>=6；
 *          table[i+1]-1 是 2^(k_i-6) 的整数倍（best_k_ 一致性约束），
 *          违反将导致 FFT 计算结果错误
 */
void lmmp_fft_tune_install_(const mp_size_t* rows, mp_size_t count);

/** @brief 恢复默认 FFT 表 */
void lmmp_fft_tune_reset_(void);

/**
 * @brief 读取默认 FFT 表
 * @param count 出参：默认表行数（不含哨兵），可为 NULL
 * @return 平铺 [阈值,k] 数组指针
 */
const mp_size_t* lmmp_fft_tune_default_rows_(mp_size_t* count);
#endif

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

/**
 * @brief ifft 蝶形数据面融合内核（两段扫描 + 连续进位链，单次调用完成）
 *
 * 以 t_i = [numb]>>shr 的逐 limb 流水线（numb 全程只读，不回写移位结果）：
 *   扫描1（i∈[0,w)）：numc[l-w+i] = numa[l-w+i] + t_i (+bcy 链)
 *                    numa[l-w+i] -= t_i (+acy 链)
 *   扫描2（i∈[0,l-w)）：numc[i] = numa[i] - t'_i (+bcy 链续)
 *                       numa[i] += t'_i (+acy 链续)，t'_i 从 numb[w] 起算
 *   两段扫描的顶 limb 分别从 numb[w]、numb[l] 取高 shr 位（内核自行加载）
 * @param numc 蝶形输出系数指针（temp_coef）
 * @param numa 系数 a 指针（原地加减）
 * @param numb 系数 b 指针（只读，须有 l+1 个 limb 可读）
 * @param l 系数长度（ms->lenw）
 * @param w 机器字级移位量（0<=w<=l）
 * @param shr 比特级右移量
 * @warning 0<shr<64, 0<=w<=l, sep(numc,[numa|numb]), l>0,
 *          各指针非空；实现见 lmmpn/generic/fft_fuse.c（C）与 asm/{x64,arm64}/fft_fuse.S
 * @return 打包进位 (acy_final << 1) | bcy_final（acy 为 numa 链，bcy 为 numc 链）
 */
mp_limb_t lmmp_ifft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb,
                               mp_size_t l, mp_size_t w, mp_size_t shr);

/**
 * @brief fft 蝶形数据面融合内核（两段扫描 + 中点 ch 分解，单次调用完成）
 *
 *   扫描1（j∈[0,l-w)）：D = numa[j] - numb[j] (-scyo 链，起点0)
 *                      numc[w+j] = (D << shl) 接 shl 移位链（起点0）
 *                      numa[j] = numa[j] + numb[j] (+acyo 链，起点0)
 *   中点：ch = shlcy1 - (scyo1 << shl) 分解为 chp/chn（scyo1/shlcy1 为扫描1终值）
 *   扫描2（j∈[0,w)）：D2 = numb[l-w+j] - numa[l-w+j] (-scyo 链重置0)
 *                    numc[j] = (D2 << shl) 接 shl 链（重置0）
 *                    numa[l-w+j] += numb[l-w+j] (+acyo 链续)
 * @param numc 蝶形输出系数指针（temp_coef）
 * @param numa 系数 a 指针（原地加）
 * @param numb 系数 b 指针（只读）
 * @param l 系数长度（ms->lenw）
 * @param w 机器字级移位量（0<=w<=l）
 * @param shl 比特级左移量
 * @param out 输出数组（至少 4 个 limb）：out[0]=chp, out[1]=chn,
 *            out[2]=shlcy2（扫描2 shl 链移出位），out[3]=(acy_final<<1)|scyo_final
 * @warning 0<shl<64, 0<=w<=l, sep(numc,[numa|numb]), l>0,
 *          各指针非空；实现见 lmmpn/generic/fft_fuse.c（C）与 asm/{x64,arm64}/fft_fuse.S
 */
void lmmp_fft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb, mp_size_t l, mp_size_t w,
                         mp_size_t shl, mp_limb_t* out);

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

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

#ifndef __LMMP_MPARAM_H__
#define __LMMP_MPARAM_H__

// 默认线程局部栈大小（不可变更），单位为字节
#define LMMP_DEFAULT_STACK_SIZE (320 * 1024)

// 线程局部内存池大小（可以为0，表示不使用线程局部内存池），单位为字节
#define LMMP_POOL_SIZE (512 * 1024)

// 除法阈值：当操作数规模超过此值时，使用分治除法算法
#define DIV_DIVIDE_THRESHOLD 50
// 乘法逆元L阈值：用于选择乘法逆元计算策略的临界值
#define DIV_MULINV_L_THRESHOLD 477
// 乘法逆元N阈值：用于选择乘法逆元计算策略的临界值
#define DIV_MULINV_N_THRESHOLD 1736

// 牛顿迭代求逆阈值：超过此规模使用牛顿迭代法求逆
#define INV_NEWTON_THRESHOLD 21
// 梅森变换求逆阈值：超过此规模使用梅森变换法求逆
#define INV_MODM_THRESHOLD 734

// 梅森变换乘法逆元阈值：超过此规模选择梅森变换计算乘法逆元
#define DIV_MULINV_MODM_THRESHOLD 477

// 平方根计算中，牛顿逆平方乘法阈值
#define SQRT_INVNEWTON_THRESHOLD 50
// 梅森变换开方阈值：超过此规模选择梅森变换计算
#define SQRT_NEWTON_MODM_THRESHOLD 734

// Toom-22乘法阈值：超过此规模使用Toom-22乘法
#define MUL_TOOM22_THRESHOLD 20
// Toom-X2乘法阈值：较短乘数小于此值使用Toom-X2不平衡乘法
#define MUL_TOOMX2_THRESHOLD 30
// Toom-33乘法阈值：超过此规模使用Toom-33乘法
#define MUL_TOOM33_THRESHOLD 65
// Toom-44乘法阈值：超过此规模使用Toom-44乘法
#define MUL_TOOM44_THRESHOLD 581
// FFT乘法阈值：超过此规模使用快速傅里叶变换(FFT)乘法
#define MUL_FFT_THRESHOLD 2316

// 低位乘法阈值：低于此规模使用朴素乘法
#define MULLO_BASECASE_THRESHOLD 20
// 低位除法阈值：低于此规模使用不平衡分治乘法
#define MULLO_DC_THRESHOLD 3521

// 精确逆元阈值：高于此规模使用牛顿迭代法
#define BNINV_NEWTON_THRESHOLD 20

// 费马变换阈值：低于此规模使用直接乘法而不再进行递归
#define MUL_FFT_MODF_THRESHOLD 477

// 转字符串除法阈值：字符串转换时选择除法算法的临界值
#define TO_STR_DIVIDE_THRESHOLD 20
// 转字符串基数幂阈值：字符串转换时基数幂计算的策略选择临界值
#define TO_STR_BASEPOW_THRESHOLD 30
// 从字符串解析除法阈值：字符串解析时选择除法算法的临界值
#define FROM_STR_DIVIDE_THRESHOLD 45
// 从字符串解析基数幂阈值：字符串解析时基数幂计算的策略选择临界值
#define FROM_STR_BASEPOW_THRESHOLD 100

// L1缓存大小，请将此值设置为实际单核CPU的L1缓存大小（字节数）
// 8192 字节通常远远小于现代CPU的L1缓存大小，主要为分块缓存大小考虑
#define L1_CACHE_SIZE 8192

// L2缓存大小，请将此值设置为实际单核CPU的L2缓存大小（字节数）
// 1Mb 字节是一个相对保守的数值
#define L2_CACHE_SIZE (1ull << 20)

#ifndef LIMB_BYTES
#define LIMB_BYTES 8
#endif

// L1缓存分块大小
#define PART_SIZE (L1_CACHE_SIZE / LIMB_BYTES / 2)

// 2x2矩阵乘法选择STRASSEN算法的阈值
#define MAT22_MUL_STRASSEN_THRESHOLD 60

// 2x2矩阵平方选择STRASSEN算法的阈值
#define MAT22_SQR_STRASSEN_THRESHOLD 50

// 幂运算中，底数长度为 1 的幂运算指数阈值，低于此阈值使用连乘法
#define POW_1_EXP_THRESHOLD 10

// 幂运算中，指数大于此值可能使用win2算法
#define POW_WIN2_EXP_THRESHOLD 50

// 幂运算中，底数长度大于此值可能使用win2算法
#define POW_WIN2_N_THRESHOLD 400

// 因子累乘中，因子数量低于此阈值则使用朴素连乘
#define FACTORS_MUL_N_THRESHOLD 30

// 排列数计算中，nPr直线分割阈值
#define PERMUTATION_USHORT_K_THRESHOLD 18
#define PERMUTATION_USHORT_B_THRESHOLD 21164
#define PERMUTATION_UINT_K_THRESHOLD 136
#define PERMUTATION_UINT_B_THRESHOLD 1659975

// 排列数计算中，结果长度小于此阈值的将使用朴素算法
#define BINOMIAL_RN_BASECASE_THRESHOLD 40
// 元素累乘中，低于此长度的累乘将使用朴素算法
#define ELEM_MUL_BASECASE_THRESHOLD 25

// 使用梅森乘法计算高位的阈值
#define MULHI_MERSENNE_THRESHOLD 477

// 精确除法中，除数小于此阈值时使用朴素法
#define DIVEXACT_BASECASE_THRESHOLD 50
// 精确除法中，被除数小于此阈值时使用朴素法
#define DIVEXACT_NN_THRESHOLD 350


// cache 一次处理的位图数量
#define PRIME_CACHE_BLOCK_NUM 64
// cache 中质数最多可能的数量（取决于上面的PRIME_CACHE_BLOCK_NUM）
#define PRIME_CACHE_SIZE 1028

#define MP_UCHAR_MAX (0xff)
#define MP_USHORT_MAX (0xffff)
#define MP_UINT_MAX (0xffffffff)
#define MP_ULONG_MAX (0xffffffffffffffffull)

#define MP_CHAR_BITS (8)
#define MP_SHORT_BITS (16)
#define MP_INT_BITS (32)
#define MP_LONG_BITS (64)

#define MP_CHAR_BYTES (1)
#define MP_SHORT_BYTES (2)
#define MP_INT_BYTES (4)
#define MP_LONG_BYTES (8)

#define ODD_FACTORIAL_SIZE 25

#define NPR_SHORT_LIMIT (0xffff)
#define NPR_INT_LIMIT (0xffffffff)

#define NCR_SHORT_LIMIT (0xffff)

// B / 2
#define LIMB_B_2 (0x8000000000000000ull)
// B / 4
#define LIMB_B_4 (0x4000000000000000ull)

#endif // __LMMP_MPARAM_H__
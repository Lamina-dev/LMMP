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
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

// 除法阈值：当操作数规模超过此值时，使用分治除法算法
#define DIV_DIVIDE_THRESHOLD 50
// 乘法逆元L阈值：用于选择乘法逆元计算策略的临界值
#define DIV_MULINV_L_THRESHOLD 427
// 乘法逆元N阈值：用于选择乘法逆元计算策略的临界值
#define DIV_MULINV_N_THRESHOLD 1736
// 牛顿迭代求逆阈值：超过此规模使用牛顿迭代法求逆
#define INV_NEWTON_THRESHOLD 21
// 梅森变换求逆阈值：超过此规模牛顿步残差积走梅森乘法（分发入口，dc/fft）
#define INV_MODM_THRESHOLD 64
// 梅森变换乘法逆元阈值：卷绕余数更新的梅森积改用缓存（仅 fft）的分界
#define DIV_MULINV_MODM_CACHE_THRESHOLD 427
// 同上场景的低档：朴素直乘改走梅森乘法（分发入口，不缓存）的分界
#define DIV_MULINV_MODM_THRESHOLD 64
// Toom-22乘法阈值：超过此规模使用Toom-22乘法
#define MUL_TOOM22_THRESHOLD 20
// Toom-33乘法阈值：超过此规模使用Toom-33乘法
#define MUL_TOOM33_THRESHOLD 115
// Toom-44乘法阈值：超过此规模使用Toom-44乘法
#define MUL_TOOM44_THRESHOLD 305
// Toom-6.5乘法阈值：超过此规模使用 toom6h
#define MUL_TOOM6H_THRESHOLD 459
// Toom-8.5乘法阈值：超过此规模使用 toom8h
#define MUL_TOOM8H_THRESHOLD 602
// 平方 Toom 阈值：平方与乘法在各 toom 层的交叉点不完全一致，分立调优。
// SQR_TOOM22 上限受硬编码平方 LMMP_MUL_HARD_MAX_N 钉死(与乘法同构)
#define SQR_TOOM22_THRESHOLD 20
#define SQR_TOOM33_THRESHOLD 268
#define SQR_TOOM44_THRESHOLD 454
// 平方 Toom-6/8 阈值
#define SQR_TOOM6_THRESHOLD 609
#define SQR_TOOM8_THRESHOLD 646
// FFT乘法阈值：超过此规模使用快速傅里叶变换(FFT)乘法
#define MUL_FFT_THRESHOLD 2345
// FFT平方阈值：超过此规模使用FFT平方。平方与乘法的 toom4/FFT 交叉点
#define SQR_FFT_THRESHOLD 1544
// 低位乘法阈值：低于此规模使用朴素乘法
#define MULLO_BASECASE_THRESHOLD 32
// 低位平方阈值：低于此规模使用朴素平方
#define SQRLO_BASECASE_THRESHOLD 32
// 低位乘法阈值：低于此规模使用分治乘法
#define MULLO_DC_THRESHOLD 3538
// 精确逆元阈值：高于此规模使用牛顿迭代法
#define BNINV_NEWTON_THRESHOLD 20
// 费马变换阈值：低于此规模使用直接乘法而不再进行递归
#define MUL_FFT_MODF_THRESHOLD 411
// 转字符串除法阈值：字符串转换时选择除法算法的临界值
#define TO_STR_DIVIDE_THRESHOLD 20
// 转字符串基数幂阈值：字符串转换时基数幂计算的策略选择临界值
#define TO_STR_BASEPOW_THRESHOLD 30
// 从字符串解析除法阈值：字符串解析时选择除法算法的临界值
#define FROM_STR_DIVIDE_THRESHOLD 45
// 从字符串解析基数幂阈值：字符串解析时基数幂计算的策略选择临界值
#define FROM_STR_BASEPOW_THRESHOLD 100
// 分治梅森乘法（lmmp_mul_mersenne_dc_）递归入口阈值：rn 为偶且不低于此值时使用
#define MUL_MERSENNE_DC_THRESHOLD 10
// 分治梅森平方（lmmp_sqr_mersenne_dc_）递归入口阈值
#define SQR_MERSENNE_DC_THRESHOLD 12
// dc 递归内 mod B^n+1 半域积切换阈值：n 不低于此值且按 best_k 粒度对齐时走
// lmmp_mul_fermat_，否则用普通乘法 + 反向折叠（本机费马引擎 <640L 相对直乘无优势）
#define MUL_MERSENNE_FERMAT_THRESHOLD 469
#define SQR_MERSENNE_FERMAT_THRESHOLD 458
// 梅森乘法分发阈值（lmmp_mul_mersenne_）：rn 低于此值走分治 dc，否则走 SSA FFT
#define MUL_MERSENNE_FFT_THRESHOLD 543
#define SQR_MERSENNE_FFT_THRESHOLD 543
// 某些计算中，一次性处理的64位数组的长度（此大小为64位数组的长度）
#define PART_SIZE 256

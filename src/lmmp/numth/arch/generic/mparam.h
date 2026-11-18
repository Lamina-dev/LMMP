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

// 平方根计算中，无余数且极度不平衡（nf >= 此值*na）时切换牛顿逆平方路径的斜率阈值
#define SQRT_INVNEWTON_K_THRESHOLD 20
// 立方根计算中牛顿路径的切换阈值（nf >= K*na 且 nf >= NF_MIN）。
#define CBRT_INVNEWTON_K_THRESHOLD 45
// 立方根牛顿路径的 nf 绝对下限（limb），封住小 na 段水平交叉点之下
#define CBRT_INVNEWTON_NF_MIN 1845
// 梅森变换开方阈值：超过此规模牛顿残差链走梅森乘法（分发入口，dc/fft）
// 此阈值与牛顿迭代中残差积的形状约束耦合。
#define SQRT_NEWTON_MODM_THRESHOLD 434
// 梅森变换开立方阈值：超过此规模牛顿残差链走梅森乘法（分发入口，dc/fft；
// cbrt_newton 层内 an*ir^3 残差链的模乘替代完整立方展开；形状守卫与该值
// 耦合）
#define CBRT_NEWTON_MODM_THRESHOLD 434
// 2x2矩阵乘法选择STRASSEN算法的阈值
#define MAT22_MUL_STRASSEN_THRESHOLD 24
// 2x2矩阵平方选择STRASSEN算法的阈值
#define MAT22_SQR_STRASSEN_THRESHOLD 24
// 幂运算中，底数长度为 1 的幂运算指数阈值，低于此阈值使用连乘法
#define POW_1_EXP_THRESHOLD 24
// 幂运算中，指数大于此值可能使用win2算法
#define POW_WIN2_EXP_THRESHOLD 128
// 幂运算中，底数长度大于此值可能使用win2算法
#define POW_WIN2_N_THRESHOLD 400
// 因子累乘中，因子数量低于此阈值则使用朴素连乘
#define FACTORS_MUL_N_THRESHOLD 30
// 排列数计算中，nPr直线分割阈值
#define PERMUTATION_USHORT_K_THRESHOLD 22
#define PERMUTATION_USHORT_B_THRESHOLD 21164
#define PERMUTATION_UINT_K_THRESHOLD 51
#define PERMUTATION_UINT_B_THRESHOLD 268247
// 排列数计算中，结果长度小于此阈值的将使用朴素算法
#define BINOMIAL_RN_BASECASE_THRESHOLD 32
// 组合数计算中，div路径(累乘nPr后精确除以r!)与factor路径(质因数分解后累乘)的分界斜率阈值
// 当 K * nPr_n > B * fac_n 时选择div路径，其中nPr_n为nPr的limb数，fac_n为r!的limb数
// 即 nPr_n / fac_n > B / K 时走div路径
#define BINOMIAL_DIV_K_THRESHOLD 50
#define BINOMIAL_DIV_B_THRESHOLD 91
// 元素累乘中，低于此长度的累乘将使用朴素算法
#define ELEM_MUL_BASECASE_THRESHOLD 32
// binvert不平衡算法中，使用梅森变换的阈值
#define BINVERT_UNBALANCE_MODM_THRESHOLD 32
// binvert不平衡算法中，使用梅森缓存变换的阈值
#define BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD 1460
// REDC 归约的 q*m 高半积中，使用梅森变换的阈值
#define REDC_MERSENNE_THRESHOLD 40
// REDC 归约的 basecase 阈值
#define REDC_BASECASE_THRESHOLD 41
// REDC 归约中 q*m 高半积改用梅森折叠提取的阈值
#define REDC_MERSENNE_CACHE_THRESHOLD 462
// gcd 算法选择阈值：较大输入长度达到此值时使用 hgcd 分治算法，否则使用 Lehmer 算法
#define GCD_HGCD_THRESHOLD 86
// 精确除法中，除数小于此阈值时使用朴素法
#define DIVEXACT_BASECASE_THRESHOLD 62
// 精确除法中，被除数小于此阈值时使用朴素法
#define DIVEXACT_NN_THRESHOLD 699
// hgcd 矩阵应用模乘改用梅森缓存的阈值
#define HGCD_MODMUL_CACHE_THRESHOLD 543
// hgcd 矩阵应用模乘的低档：朴素调整改走梅森乘法（分发入口，不缓存）
#define HGCD_MODMUL_THRESHOLD 64
// divexact mu 主循环中，使用梅森变换的阈值
#define DIVEXACT_MERSENNE_THRESHOLD 1540
// divexact mu 主循环中，使用梅森缓存变换的阈值
#define DIVEXACT_MERSENNE_CACHE_THRESHOLD 1540
// 试除法分派阈值：nn 低于此值回退 mod_preinv_1_ 腿（块积预处理全局表复用）
#define TRIALDIV_PREINV_THRESHOLD 12
// 试除法分派阈值：nn 达到此值起 8 链折叠取模，中间段 4 链
#define TRIALDIV_K8_THRESHOLD 192
#define ODD_FACTORIAL_SIZE 25
#define NPR_SHORT_LIMIT (0xffff)
#define NPR_INT_LIMIT (0xffffffff)
#define NCR_SHORT_LIMIT (0xffff)
// 使用梅森乘法计算高位的阈值
#define MULHI_MERSENNE_THRESHOLD 1019

// cache 一次处理的位图数量
#define PRIME_CACHE_BLOCK_NUM 64
// cache 中质数最多可能的数量（取决于上面的PRIME_CACHE_BLOCK_NUM）。
// mod-30 位图下 64 字 = 512 字节 = 15360 个数，最坏为首块（含补入的 3、5）：
// pi(15359) - 1 = 1793，取 1794（与旧奇数编码取 pi(8191) = 1028 同风格 +1 松弛）
#define PRIME_CACHE_SIZE 1794

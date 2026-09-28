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

#ifndef __LMMP_MPARAM_H__
#define __LMMP_MPARAM_H__

// 默认线程局部栈大小（不可变更），单位为字节
#define LMMP_DEFAULT_STACK_SIZE (320 * 1024)

// 线程局部内存池大小（可以为0，表示不使用线程局部内存池），单位为字节
#define LMMP_POOL_SIZE (512 * 1024)

// 除法阈值：当操作数规模超过此值时，使用分治除法算法
#define LMMP_DEFAULT_DIV_DIVIDE_THRESHOLD 50
#ifdef LMMP_TUNE
#define DIV_DIVIDE_THRESHOLD lmmp_tune_DIV_DIVIDE_THRESHOLD
#else
#define DIV_DIVIDE_THRESHOLD LMMP_DEFAULT_DIV_DIVIDE_THRESHOLD
#endif
// 乘法逆元L阈值：用于选择乘法逆元计算策略的临界值
#define DIV_MULINV_L_THRESHOLD 427
// 乘法逆元N阈值：用于选择乘法逆元计算策略的临界值
#define DIV_MULINV_N_THRESHOLD 1736

// 牛顿迭代求逆阈值：超过此规模使用牛顿迭代法求逆
#define INV_NEWTON_THRESHOLD 21
// 梅森变换求逆阈值：超过此规模使用梅森变换法求逆
#define INV_MODM_THRESHOLD 734

// 梅森变换乘法逆元阈值：超过此规模选择梅森变换计算乘法逆元
#define DIV_MULINV_MODM_THRESHOLD 427

// 平方根计算中，无余数且极度不平衡（nf >= 此值*na）时切换牛顿逆平方路径的斜率阈值
#define LMMP_DEFAULT_SQRT_INVNEWTON_K_THRESHOLD 20
#ifdef LMMP_TUNE
#define SQRT_INVNEWTON_K_THRESHOLD lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD
#else
#define SQRT_INVNEWTON_K_THRESHOLD LMMP_DEFAULT_SQRT_INVNEWTON_K_THRESHOLD
#endif

/*
    立方根牛顿路径的切换阈值（nf >= K*na 且 nf >= NF_MIN）。

    lmmp_cbrt_newton_ 与 lmmp_cbrt_divide_ 的等耗时边界实测（Apple M 系列
    arm64，Release，中位数计时，na*nf 网格线性插值）：

        na:      3     6    12    24    48    96   192   384    768
        nf*:  ~2100 ~1960 ~2000 ~2670 ~3320 ~5310 ~10000 ~21900  >64000
        斜率:   -     -     -   111    69    55    52    57     >83

    即边界在 na <= 12 为 ~2000 的水平段（newton 的固定开销主导：cbrt 重建
    需两次全乘 Q=a2*ic^2 且层修正含 div_1 除 3，均非 sqrt 的移位可比），
    在 na ∈ [48,384] 为过原点射线 nf* ≈ 52*na（两算法的 na 增量成本比），
    na >= 768 后上翘（invcbrt 层内 an 参与的大层残差乘使 newton 的 na 敏感
    度超过 divide 的 3*ns 增量）。故切换条件取 AND 复合：射线项 K*na 覆盖
    中段（纯 sqrt 式），绝对下限 NF_MIN 封住水平段之下 newton 恒慢的小 nf
    区（na<=12 时 K*na 远小于真实交叉点，纯射线会在 nf≈250~1500 处误选
    newton，实测慢 15%~80%）。K 略偏保守（实测中段 52~57，取 52）：
    na >= 768 的上翘段会在真实交叉点前 ~5% 提前切入 newton，属可接受损失。
    sqrt 对照实测（同法）：其交叉点为 nf* ≈ 300（na<=24）过渡到斜率 ~16
    的射线（na ∈ [96,384]），水平段仅 ~300 limb，纯射线 SQRT_INVNEWTON
    _K_THRESHOLD=20 误选区间小（nf ∈ [20*na, nf*] 内损失有限）故无需
    NF_MIN；cbrt 水平段 ~2000 limb 是 sqrt 的 ~7 倍，误选损失大，必须复合。
*/
#define LMMP_DEFAULT_CBRT_INVNEWTON_K_THRESHOLD 52
#ifdef LMMP_TUNE
#define CBRT_INVNEWTON_K_THRESHOLD lmmp_tune_CBRT_INVNEWTON_K_THRESHOLD
#else
#define CBRT_INVNEWTON_K_THRESHOLD LMMP_DEFAULT_CBRT_INVNEWTON_K_THRESHOLD
#endif
// 立方根牛顿路径的 nf 绝对下限（limb），封住小 na 段水平交叉点之下
#define CBRT_INVNEWTON_NF_MIN 2000
// 梅森变换开方阈值：超过此规模选择梅森变换计算
#define SQRT_NEWTON_MODM_THRESHOLD 434

// 梅森变换开立方阈值：超过此规模选择梅森变换计算（cbrt_newton 层内
// an*ir^3 残差链的模乘替代完整立方展开）
#define CBRT_NEWTON_MODM_THRESHOLD 434

// Toom-22乘法阈值：超过此规模使用Toom-22乘法
#define LMMP_DEFAULT_MUL_TOOM22_THRESHOLD 20
#ifdef LMMP_TUNE
#define MUL_TOOM22_THRESHOLD lmmp_tune_MUL_TOOM22_THRESHOLD
#else
#define MUL_TOOM22_THRESHOLD LMMP_DEFAULT_MUL_TOOM22_THRESHOLD
#endif
// Toom-33乘法阈值：超过此规模使用Toom-33乘法
#define LMMP_DEFAULT_MUL_TOOM33_THRESHOLD 115
#ifdef LMMP_TUNE
#define MUL_TOOM33_THRESHOLD lmmp_tune_MUL_TOOM33_THRESHOLD
#else
#define MUL_TOOM33_THRESHOLD LMMP_DEFAULT_MUL_TOOM33_THRESHOLD
#endif
// Toom-44乘法阈值：超过此规模使用Toom-44乘法
#define LMMP_DEFAULT_MUL_TOOM44_THRESHOLD 307
#ifdef LMMP_TUNE
#define MUL_TOOM44_THRESHOLD lmmp_tune_MUL_TOOM44_THRESHOLD
#else
#define MUL_TOOM44_THRESHOLD LMMP_DEFAULT_MUL_TOOM44_THRESHOLD
#endif

// 平方 Toom 阈值：平方与乘法在各 toom 层的交叉点不完全一致，分立调优。
// SQR_TOOM22 上限受硬编码平方 LMMP_MUL_HARD_MAX_N 钉死(与乘法同构)
#define LMMP_DEFAULT_SQR_TOOM22_THRESHOLD 20
#ifdef LMMP_TUNE
#define SQR_TOOM22_THRESHOLD lmmp_tune_SQR_TOOM22_THRESHOLD
#else
#define SQR_TOOM22_THRESHOLD LMMP_DEFAULT_SQR_TOOM22_THRESHOLD
#endif
#define LMMP_DEFAULT_SQR_TOOM33_THRESHOLD 268
#ifdef LMMP_TUNE
#define SQR_TOOM33_THRESHOLD lmmp_tune_SQR_TOOM33_THRESHOLD
#else
#define SQR_TOOM33_THRESHOLD LMMP_DEFAULT_SQR_TOOM33_THRESHOLD
#endif
#define LMMP_DEFAULT_SQR_TOOM44_THRESHOLD 381
#ifdef LMMP_TUNE
#define SQR_TOOM44_THRESHOLD lmmp_tune_SQR_TOOM44_THRESHOLD
#else
#define SQR_TOOM44_THRESHOLD LMMP_DEFAULT_SQR_TOOM44_THRESHOLD
#endif
// FFT乘法阈值：超过此规模使用快速傅里叶变换(FFT)乘法
#define LMMP_DEFAULT_MUL_FFT_THRESHOLD 1981
#ifdef LMMP_TUNE
#define MUL_FFT_THRESHOLD lmmp_tune_MUL_FFT_THRESHOLD
#else
#define MUL_FFT_THRESHOLD LMMP_DEFAULT_MUL_FFT_THRESHOLD
#endif

// FFT平方阈值：超过此规模使用FFT平方。平方与乘法的 toom4/FFT 交叉点
// 并不一致（平方交叉点显著更早），故与 MUL_FFT_THRESHOLD 分立调优
#define LMMP_DEFAULT_SQR_FFT_THRESHOLD 1405
#ifdef LMMP_TUNE
#define SQR_FFT_THRESHOLD lmmp_tune_SQR_FFT_THRESHOLD
#else
#define SQR_FFT_THRESHOLD LMMP_DEFAULT_SQR_FFT_THRESHOLD
#endif

// 不平衡乘法硬编码分块阈值：na > PART_SIZE 且较短乘数长度在
// [此值, LMMP_MUL_HARD_MAX_N] 时，mul_basecase_unbalanced 以短乘数长度分块
// 并调用硬编码平衡乘累加；其余情形(短乘数过细分块或 na 较小单次直达更优)
// 走原 mul_basecase 逐列或 PART_SIZE 分块路径
#define LMMP_DEFAULT_MUL_UNBALANCED_HARD_THRESHOLD 7
#ifdef LMMP_TUNE
#define MUL_UNBALANCED_HARD_THRESHOLD lmmp_tune_MUL_UNBALANCED_HARD_THRESHOLD
#else
#define MUL_UNBALANCED_HARD_THRESHOLD LMMP_DEFAULT_MUL_UNBALANCED_HARD_THRESHOLD
#endif

// 低位乘法阈值：低于此规模使用朴素乘法
#define LMMP_DEFAULT_MULLO_BASECASE_THRESHOLD 48
#ifdef LMMP_TUNE
#define MULLO_BASECASE_THRESHOLD lmmp_tune_MULLO_BASECASE_THRESHOLD
#else
#define MULLO_BASECASE_THRESHOLD LMMP_DEFAULT_MULLO_BASECASE_THRESHOLD
#endif
// 低位乘法阈值：低于此规模使用分治乘法
#define LMMP_DEFAULT_MULLO_DC_THRESHOLD 3521
#ifdef LMMP_TUNE
#define MULLO_DC_THRESHOLD lmmp_tune_MULLO_DC_THRESHOLD
#else
#define MULLO_DC_THRESHOLD LMMP_DEFAULT_MULLO_DC_THRESHOLD
#endif

// 精确逆元阈值：高于此规模使用牛顿迭代法
#define LMMP_DEFAULT_BNINV_NEWTON_THRESHOLD 20
#ifdef LMMP_TUNE
#define BNINV_NEWTON_THRESHOLD lmmp_tune_BNINV_NEWTON_THRESHOLD
#else
#define BNINV_NEWTON_THRESHOLD LMMP_DEFAULT_BNINV_NEWTON_THRESHOLD
#endif

// 费马变换阈值：低于此规模使用直接乘法而不再进行递归
#define LMMP_DEFAULT_MUL_FFT_MODF_THRESHOLD 411
#ifdef LMMP_TUNE
#define MUL_FFT_MODF_THRESHOLD lmmp_tune_MUL_FFT_MODF_THRESHOLD
#else
#define MUL_FFT_MODF_THRESHOLD LMMP_DEFAULT_MUL_FFT_MODF_THRESHOLD
#endif

// 转字符串除法阈值：字符串转换时选择除法算法的临界值
#define LMMP_DEFAULT_TO_STR_DIVIDE_THRESHOLD 20
#ifdef LMMP_TUNE
#define TO_STR_DIVIDE_THRESHOLD lmmp_tune_TO_STR_DIVIDE_THRESHOLD
#else
#define TO_STR_DIVIDE_THRESHOLD LMMP_DEFAULT_TO_STR_DIVIDE_THRESHOLD
#endif
// 转字符串基数幂阈值：字符串转换时基数幂计算的策略选择临界值
#define LMMP_DEFAULT_TO_STR_BASEPOW_THRESHOLD 30
#ifdef LMMP_TUNE
#define TO_STR_BASEPOW_THRESHOLD lmmp_tune_TO_STR_BASEPOW_THRESHOLD
#else
#define TO_STR_BASEPOW_THRESHOLD LMMP_DEFAULT_TO_STR_BASEPOW_THRESHOLD
#endif
// 从字符串解析除法阈值：字符串解析时选择除法算法的临界值
#define LMMP_DEFAULT_FROM_STR_DIVIDE_THRESHOLD 45
#ifdef LMMP_TUNE
#define FROM_STR_DIVIDE_THRESHOLD lmmp_tune_FROM_STR_DIVIDE_THRESHOLD
#else
#define FROM_STR_DIVIDE_THRESHOLD LMMP_DEFAULT_FROM_STR_DIVIDE_THRESHOLD
#endif
// 从字符串解析基数幂阈值：字符串解析时基数幂计算的策略选择临界值
#define LMMP_DEFAULT_FROM_STR_BASEPOW_THRESHOLD 100
#ifdef LMMP_TUNE
#define FROM_STR_BASEPOW_THRESHOLD lmmp_tune_FROM_STR_BASEPOW_THRESHOLD
#else
#define FROM_STR_BASEPOW_THRESHOLD LMMP_DEFAULT_FROM_STR_BASEPOW_THRESHOLD
#endif

// 2x2矩阵乘法选择STRASSEN算法的阈值
#define LMMP_DEFAULT_MAT22_MUL_STRASSEN_THRESHOLD 24
#ifdef LMMP_TUNE
#define MAT22_MUL_STRASSEN_THRESHOLD lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD
#else
#define MAT22_MUL_STRASSEN_THRESHOLD LMMP_DEFAULT_MAT22_MUL_STRASSEN_THRESHOLD
#endif

// 2x2矩阵平方选择STRASSEN算法的阈值
#define LMMP_DEFAULT_MAT22_SQR_STRASSEN_THRESHOLD 24
#ifdef LMMP_TUNE
#define MAT22_SQR_STRASSEN_THRESHOLD lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD
#else
#define MAT22_SQR_STRASSEN_THRESHOLD LMMP_DEFAULT_MAT22_SQR_STRASSEN_THRESHOLD
#endif

// 幂运算中，底数长度为 1 的幂运算指数阈值，低于此阈值使用连乘法
#define LMMP_DEFAULT_POW_1_EXP_THRESHOLD 24
#ifdef LMMP_TUNE
#define POW_1_EXP_THRESHOLD lmmp_tune_POW_1_EXP_THRESHOLD
#else
#define POW_1_EXP_THRESHOLD LMMP_DEFAULT_POW_1_EXP_THRESHOLD
#endif

// 幂运算中，指数大于此值可能使用win2算法
#define LMMP_DEFAULT_POW_WIN2_EXP_THRESHOLD 24
#ifdef LMMP_TUNE
#define POW_WIN2_EXP_THRESHOLD lmmp_tune_POW_WIN2_EXP_THRESHOLD
#else
#define POW_WIN2_EXP_THRESHOLD LMMP_DEFAULT_POW_WIN2_EXP_THRESHOLD
#endif

// 幂运算中，底数长度大于此值可能使用win2算法
#define LMMP_DEFAULT_POW_WIN2_N_THRESHOLD 400
#ifdef LMMP_TUNE
#define POW_WIN2_N_THRESHOLD lmmp_tune_POW_WIN2_N_THRESHOLD
#else
#define POW_WIN2_N_THRESHOLD LMMP_DEFAULT_POW_WIN2_N_THRESHOLD
#endif

// 因子累乘中，因子数量低于此阈值则使用朴素连乘
#define LMMP_DEFAULT_FACTORS_MUL_N_THRESHOLD 30
#ifdef LMMP_TUNE
#define FACTORS_MUL_N_THRESHOLD lmmp_tune_FACTORS_MUL_N_THRESHOLD
#else
#define FACTORS_MUL_N_THRESHOLD LMMP_DEFAULT_FACTORS_MUL_N_THRESHOLD
#endif

// 排列数计算中，nPr直线分割阈值
#define LMMP_DEFAULT_PERMUTATION_USHORT_K_THRESHOLD 22
#ifdef LMMP_TUNE
#define PERMUTATION_USHORT_K_THRESHOLD lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD
#else
#define PERMUTATION_USHORT_K_THRESHOLD LMMP_DEFAULT_PERMUTATION_USHORT_K_THRESHOLD
#endif
#define LMMP_DEFAULT_PERMUTATION_USHORT_B_THRESHOLD 21164
#ifdef LMMP_TUNE
#define PERMUTATION_USHORT_B_THRESHOLD lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD
#else
#define PERMUTATION_USHORT_B_THRESHOLD LMMP_DEFAULT_PERMUTATION_USHORT_B_THRESHOLD
#endif
#define LMMP_DEFAULT_PERMUTATION_UINT_K_THRESHOLD 51
#ifdef LMMP_TUNE
#define PERMUTATION_UINT_K_THRESHOLD lmmp_tune_PERMUTATION_UINT_K_THRESHOLD
#else
#define PERMUTATION_UINT_K_THRESHOLD LMMP_DEFAULT_PERMUTATION_UINT_K_THRESHOLD
#endif
#define LMMP_DEFAULT_PERMUTATION_UINT_B_THRESHOLD 268247
#ifdef LMMP_TUNE
#define PERMUTATION_UINT_B_THRESHOLD lmmp_tune_PERMUTATION_UINT_B_THRESHOLD
#else
#define PERMUTATION_UINT_B_THRESHOLD LMMP_DEFAULT_PERMUTATION_UINT_B_THRESHOLD
#endif

// 排列数计算中，结果长度小于此阈值的将使用朴素算法
#define LMMP_DEFAULT_BINOMIAL_RN_BASECASE_THRESHOLD 32
#ifdef LMMP_TUNE
#define BINOMIAL_RN_BASECASE_THRESHOLD lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD
#else
#define BINOMIAL_RN_BASECASE_THRESHOLD LMMP_DEFAULT_BINOMIAL_RN_BASECASE_THRESHOLD
#endif

// 组合数计算中，div路径(累乘nPr后精确除以r!)与factor路径(质因数分解后累乘)的分界斜率阈值
// 当 K * nPr_n > B * fac_n 时选择div路径，其中nPr_n为nPr的limb数，fac_n为r!的limb数
// 即 nPr_n / fac_n > B / K 时走div路径
#define LMMP_DEFAULT_BINOMIAL_DIV_K_THRESHOLD 50
#ifdef LMMP_TUNE
#define BINOMIAL_DIV_K_THRESHOLD lmmp_tune_BINOMIAL_DIV_K_THRESHOLD
#else
#define BINOMIAL_DIV_K_THRESHOLD LMMP_DEFAULT_BINOMIAL_DIV_K_THRESHOLD
#endif
#define LMMP_DEFAULT_BINOMIAL_DIV_B_THRESHOLD 91
#ifdef LMMP_TUNE
#define BINOMIAL_DIV_B_THRESHOLD lmmp_tune_BINOMIAL_DIV_B_THRESHOLD
#else
#define BINOMIAL_DIV_B_THRESHOLD LMMP_DEFAULT_BINOMIAL_DIV_B_THRESHOLD
#endif
// 元素累乘中，低于此长度的累乘将使用朴素算法
#define LMMP_DEFAULT_ELEM_MUL_BASECASE_THRESHOLD 32
#ifdef LMMP_TUNE
#define ELEM_MUL_BASECASE_THRESHOLD lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD
#else
#define ELEM_MUL_BASECASE_THRESHOLD LMMP_DEFAULT_ELEM_MUL_BASECASE_THRESHOLD
#endif

// 使用梅森乘法计算高位的阈值
#define LMMP_DEFAULT_MULHI_MERSENNE_THRESHOLD 401
#ifdef LMMP_TUNE
#define MULHI_MERSENNE_THRESHOLD lmmp_tune_MULHI_MERSENNE_THRESHOLD
#else
#define MULHI_MERSENNE_THRESHOLD LMMP_DEFAULT_MULHI_MERSENNE_THRESHOLD
#endif

// gcd 算法选择阈值：较大输入长度达到此值时使用 hgcd 分治算法，否则使用 Lehmer 算法
#define LMMP_DEFAULT_GCD_HGCD_THRESHOLD 86
#ifdef LMMP_TUNE
#define GCD_HGCD_THRESHOLD lmmp_tune_GCD_HGCD_THRESHOLD
#else
#define GCD_HGCD_THRESHOLD LMMP_DEFAULT_GCD_HGCD_THRESHOLD
#endif

// 精确除法中，除数小于此阈值时使用朴素法
#define LMMP_DEFAULT_DIVEXACT_BASECASE_THRESHOLD 50
#ifdef LMMP_TUNE
#define DIVEXACT_BASECASE_THRESHOLD lmmp_tune_DIVEXACT_BASECASE_THRESHOLD
#else
#define DIVEXACT_BASECASE_THRESHOLD LMMP_DEFAULT_DIVEXACT_BASECASE_THRESHOLD
#endif
// 精确除法中，被除数小于此阈值时使用朴素法
#define LMMP_DEFAULT_DIVEXACT_NN_THRESHOLD 350
#ifdef LMMP_TUNE
#define DIVEXACT_NN_THRESHOLD lmmp_tune_DIVEXACT_NN_THRESHOLD
#else
#define DIVEXACT_NN_THRESHOLD LMMP_DEFAULT_DIVEXACT_NN_THRESHOLD
#endif

#ifdef LMMP_TUNE
#include <stdint.h>
/* 可调阈值运行时绑定（由 tune/lmmp/src/lmmp_tune_params.c 定义）。 */
extern uint64_t lmmp_tune_MUL_TOOM22_THRESHOLD;
extern uint64_t lmmp_tune_MUL_TOOM33_THRESHOLD;
extern uint64_t lmmp_tune_SQR_TOOM22_THRESHOLD;
extern uint64_t lmmp_tune_SQR_TOOM33_THRESHOLD;
extern uint64_t lmmp_tune_SQR_TOOM44_THRESHOLD;
extern uint64_t lmmp_tune_MUL_TOOM44_THRESHOLD;
extern uint64_t lmmp_tune_MUL_FFT_THRESHOLD;
extern uint64_t lmmp_tune_SQR_FFT_THRESHOLD;
extern uint64_t lmmp_tune_MUL_UNBALANCED_HARD_THRESHOLD;
extern uint64_t lmmp_tune_MULLO_BASECASE_THRESHOLD;
extern uint64_t lmmp_tune_MULLO_DC_THRESHOLD;
extern uint64_t lmmp_tune_DIV_DIVIDE_THRESHOLD;
extern uint64_t lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD;
extern uint64_t lmmp_tune_CBRT_INVNEWTON_K_THRESHOLD;
extern uint64_t lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD;
extern uint64_t lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD;
extern uint64_t lmmp_tune_PERMUTATION_UINT_K_THRESHOLD;
extern uint64_t lmmp_tune_PERMUTATION_UINT_B_THRESHOLD;
extern uint64_t lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD;
extern uint64_t lmmp_tune_BINOMIAL_DIV_K_THRESHOLD;
extern uint64_t lmmp_tune_BINOMIAL_DIV_B_THRESHOLD;
extern uint64_t lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD;
extern uint64_t lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD;
extern uint64_t lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD;
extern uint64_t lmmp_tune_POW_1_EXP_THRESHOLD;
extern uint64_t lmmp_tune_POW_WIN2_EXP_THRESHOLD;
extern uint64_t lmmp_tune_POW_WIN2_N_THRESHOLD;
extern uint64_t lmmp_tune_FACTORS_MUL_N_THRESHOLD;
extern uint64_t lmmp_tune_BNINV_NEWTON_THRESHOLD;
extern uint64_t lmmp_tune_MUL_FFT_MODF_THRESHOLD;
extern uint64_t lmmp_tune_TO_STR_DIVIDE_THRESHOLD;
extern uint64_t lmmp_tune_TO_STR_BASEPOW_THRESHOLD;
extern uint64_t lmmp_tune_FROM_STR_DIVIDE_THRESHOLD;
extern uint64_t lmmp_tune_FROM_STR_BASEPOW_THRESHOLD;
extern uint64_t lmmp_tune_MULHI_MERSENNE_THRESHOLD;
extern uint64_t lmmp_tune_DIVEXACT_BASECASE_THRESHOLD;
extern uint64_t lmmp_tune_DIVEXACT_NN_THRESHOLD;
extern uint64_t lmmp_tune_GCD_HGCD_THRESHOLD;
#endif

// 某些计算中，一次性处理的64位数组的长度（此大小为64位数组的长度）
#define PART_SIZE 256

// cache 一次处理的位图数量
#define PRIME_CACHE_BLOCK_NUM 64
// cache 中质数最多可能的数量（取决于上面的PRIME_CACHE_BLOCK_NUM）
#define PRIME_CACHE_SIZE 1028

#define MP_UCHAR_MAX (0xff)
#define MP_USHORT_MAX (0xffff)
#define MP_UINT_MAX (0xffffffff)
#define MP_ULONG_MAX (0xffffffffffffffffull)

#define MP_SIZE_MAX ((mp_size_t)0xffffffffffffffffULL)

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
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

/*
 * LMMP_TUNE 运行时阈值定义。
 *
 * 这些变量只在 LMMP_TUNE 构建中链接进调优驱动；库本体（lmmp_tune_core）
 * 通过 include/lmmp/impl/mparam.h 中的 extern 声明引用这些符号。
 */

#include <stdint.h>

uint64_t lmmp_tune_MUL_TOOM22_THRESHOLD = 20;
uint64_t lmmp_tune_MUL_TOOM33_THRESHOLD = 65;
uint64_t lmmp_tune_MUL_TOOM44_THRESHOLD = 481;
uint64_t lmmp_tune_MUL_FFT_THRESHOLD = 2316;
uint64_t lmmp_tune_MULLO_BASECASE_THRESHOLD = 20;
uint64_t lmmp_tune_MULLO_DC_THRESHOLD = 3521;
uint64_t lmmp_tune_DIV_DIVIDE_THRESHOLD = 50;
uint64_t lmmp_tune_SQRT_INVNEWTON_THRESHOLD = 50;
uint64_t lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD = 18;
uint64_t lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD = 21164;
uint64_t lmmp_tune_PERMUTATION_UINT_K_THRESHOLD = 136;
uint64_t lmmp_tune_PERMUTATION_UINT_B_THRESHOLD = 1659975;
uint64_t lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD = 40;
uint64_t lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD = 25;
uint64_t lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = 60;
uint64_t lmmp_tune_MAT22_SQR_STRASSEN_THRESHOLD = 50;
uint64_t lmmp_tune_POW_1_EXP_THRESHOLD = 10;
uint64_t lmmp_tune_POW_WIN2_EXP_THRESHOLD = 50;
uint64_t lmmp_tune_POW_WIN2_N_THRESHOLD = 400;
uint64_t lmmp_tune_FACTORS_MUL_N_THRESHOLD = 30;
uint64_t lmmp_tune_BNINV_NEWTON_THRESHOLD = 20;
uint64_t lmmp_tune_MUL_FFT_MODF_THRESHOLD = 427;
uint64_t lmmp_tune_TO_STR_DIVIDE_THRESHOLD = 20;
uint64_t lmmp_tune_TO_STR_BASEPOW_THRESHOLD = 30;
uint64_t lmmp_tune_FROM_STR_DIVIDE_THRESHOLD = 45;
uint64_t lmmp_tune_FROM_STR_BASEPOW_THRESHOLD = 100;
uint64_t lmmp_tune_MULHI_MERSENNE_THRESHOLD = 427;
uint64_t lmmp_tune_DIVEXACT_BASECASE_THRESHOLD = 50;
uint64_t lmmp_tune_DIVEXACT_NN_THRESHOLD = 350;

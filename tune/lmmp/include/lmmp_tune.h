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

#ifndef LMMP_TUNE_H
#define LMMP_TUNE_H

#include <stdint.h>

/*
 * 全部可调阈值的运行时变量。此清单必须与 include/lmmp/impl/mparam.h 的
 * extern 声明块、tune/lmmp/src/lmmp_tune_params.c 的定义保持一致；
 * 新增阈值时三处同步（mparam.h 为唯一真源，本文件按其顺序维护）。
 */

extern uint64_t lmmp_tune_MUL_TOOM22_THRESHOLD;
extern uint64_t lmmp_tune_MUL_TOOM33_THRESHOLD;
extern uint64_t lmmp_tune_SQR_TOOM22_THRESHOLD;
extern uint64_t lmmp_tune_SQR_TOOM33_THRESHOLD;
extern uint64_t lmmp_tune_SQR_TOOM44_THRESHOLD;
extern uint64_t lmmp_tune_MUL_TOOM44_THRESHOLD;
extern uint64_t lmmp_tune_MUL_TOOM6H_THRESHOLD;
extern uint64_t lmmp_tune_MUL_TOOM8H_THRESHOLD;
extern uint64_t lmmp_tune_SQR_TOOM6_THRESHOLD;
extern uint64_t lmmp_tune_SQR_TOOM8_THRESHOLD;
extern uint64_t lmmp_tune_MUL_FFT_THRESHOLD;
extern uint64_t lmmp_tune_SQR_FFT_THRESHOLD;
extern uint64_t lmmp_tune_MUL_UNBALANCED_HARD_THRESHOLD;
extern uint64_t lmmp_tune_MULLO_BASECASE_THRESHOLD;
extern uint64_t lmmp_tune_MULLO_DC_THRESHOLD;
extern uint64_t lmmp_tune_DIV_DIVIDE_THRESHOLD;
extern uint64_t lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD;
extern uint64_t lmmp_tune_CBRT_INVNEWTON_K_THRESHOLD;
extern uint64_t lmmp_tune_CBRT_INVNEWTON_NF_MIN;
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
extern uint64_t lmmp_tune_BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD;
extern uint64_t lmmp_tune_REDC_BASECASE_THRESHOLD;
extern uint64_t lmmp_tune_REDC_MERSENNE_CACHE_THRESHOLD;
extern uint64_t lmmp_tune_DIVEXACT_BASECASE_THRESHOLD;
extern uint64_t lmmp_tune_DIVEXACT_NN_THRESHOLD;
extern uint64_t lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD;
extern uint64_t lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD;
extern uint64_t lmmp_tune_REDC_MERSENNE_THRESHOLD;
extern uint64_t lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD;
extern uint64_t lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD;
extern uint64_t lmmp_tune_DIV_MULINV_MODM_THRESHOLD;
extern uint64_t lmmp_tune_HGCD_MODMUL_CACHE_THRESHOLD;
extern uint64_t lmmp_tune_HGCD_MODMUL_THRESHOLD;
extern uint64_t lmmp_tune_INV_MODM_THRESHOLD;
extern uint64_t lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD;
extern uint64_t lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD;
extern uint64_t lmmp_tune_MUL_MERSENNE_DC_THRESHOLD;
extern uint64_t lmmp_tune_SQR_MERSENNE_DC_THRESHOLD;
extern uint64_t lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD;
extern uint64_t lmmp_tune_SQR_MERSENNE_FERMAT_THRESHOLD;
extern uint64_t lmmp_tune_MUL_MERSENNE_FFT_THRESHOLD;
extern uint64_t lmmp_tune_SQR_MERSENNE_FFT_THRESHOLD;
extern uint64_t lmmp_tune_GCD_HGCD_THRESHOLD;

/*
 * 阈值变量登记表：快照/回滚用（见 lmmp_tune_params.c）。表中 name 即
 * mparam.h 的 LMMP_DEFAULT_ 宏名后缀，与 tune_record_add 的 macro_name
 * 同一命名空间，用于识别模块产出的调优结果。
 */
typedef struct {
    uint64_t* addr;
    const char* name;
} lmmp_tune_param_t;

const lmmp_tune_param_t* lmmp_tune_param_table(size_t* count);

#endif /* LMMP_TUNE_H */

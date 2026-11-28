/**
 *  Copyright (C) 2026 HJimmyK (Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
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

#ifndef LMMP_TUNE_PARAMS_H
#define LMMP_TUNE_PARAMS_H

#include "lmmp/lmmp.h"

#ifdef __cplusplus
extern "C" {
#endif

LMMP_API extern mp_size_t lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_MUL_TOOM22_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_SQR_TOOM22_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_MULLO_BASECASE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_SQRLO_BASECASE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_MULLO_DC_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_SQRLO_DC_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_MUL_FFT_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_SQR_FFT_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_DIV_DIVIDE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_DIV_MULINV_L_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_DIV_MULINV_N_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_INV_NEWTON_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_INV_MODM_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_DIV_MULINV_MODM_CACHE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_DIV_MULINV_MODM_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_MUL_MERSENNE_DC_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_SQR_MERSENNE_DC_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_MUL_MERSENNE_FERMAT_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_SQR_MERSENNE_FERMAT_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_MUL_MERSENNE_FFT_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_SQR_MERSENNE_FFT_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_FROM_STR_BASEPOW_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_FROM_STR_DIVIDE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_TO_STR_BASEPOW_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_TO_STR_DIVIDE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_DIVEXACT_BASECASE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_MULHI_MERSENNE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD;
LMMP_API extern double lmmp_tune_CBRT_INVNEWTON_EXP_SCALE;
LMMP_API extern double lmmp_tune_CBRT_INVNEWTON_MIN_ROOT;
LMMP_API extern mp_size_t lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_FACTORS_MUL_N_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_ELEM_MUL_BASECASE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_PERMUTATION_UINT_K_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_PERMUTATION_UINT_B_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_BINOMIAL_DIV_K_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_POW_1_EXP_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_POW_WIN2_EXP_THRESHOLD;
LMMP_API extern mp_size_t lmmp_tune_POW_WIN2_N_THRESHOLD;

#ifdef __cplusplus
}
#endif

#endif

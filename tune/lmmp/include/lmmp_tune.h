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

#ifndef LMMP_TUNE_H
#define LMMP_TUNE_H

#include <stdint.h>

extern uint64_t lmmp_tune_MUL_TOOM22_THRESHOLD;
extern uint64_t lmmp_tune_MUL_TOOM33_THRESHOLD;
extern uint64_t lmmp_tune_MUL_TOOM44_THRESHOLD;
extern uint64_t lmmp_tune_MUL_FFT_THRESHOLD;
extern uint64_t lmmp_tune_MULLO_BASECASE_THRESHOLD;
extern uint64_t lmmp_tune_MULLO_DC_THRESHOLD;
extern uint64_t lmmp_tune_DIV_DIVIDE_THRESHOLD;
extern uint64_t lmmp_tune_SQRT_INVNEWTON_THRESHOLD;
extern uint64_t lmmp_tune_PERMUTATION_USHORT_K_THRESHOLD;
extern uint64_t lmmp_tune_PERMUTATION_USHORT_B_THRESHOLD;
extern uint64_t lmmp_tune_PERMUTATION_UINT_K_THRESHOLD;
extern uint64_t lmmp_tune_PERMUTATION_UINT_B_THRESHOLD;
extern uint64_t lmmp_tune_BINOMIAL_RN_BASECASE_THRESHOLD;
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

#endif /* LMMP_TUNE_H */

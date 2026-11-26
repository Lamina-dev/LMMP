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

#ifndef LMMP_NUMTH_TUNE_MPARAM_H
#define LMMP_NUMTH_TUNE_MPARAM_H

#include "tune/params.h"

#if defined(LMMP_TUNE) && !defined(LMMP_TUNE_PARAMS_SOURCE)
#undef MAT22_MUL_STRASSEN_THRESHOLD
#define MAT22_MUL_STRASSEN_THRESHOLD lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD
#undef DIVEXACT_BASECASE_THRESHOLD
#define DIVEXACT_BASECASE_THRESHOLD lmmp_tune_DIVEXACT_BASECASE_THRESHOLD
#undef DIVEXACT_MERSENNE_THRESHOLD
#define DIVEXACT_MERSENNE_THRESHOLD lmmp_tune_DIVEXACT_MERSENNE_THRESHOLD
#undef DIVEXACT_MERSENNE_CACHE_THRESHOLD
#define DIVEXACT_MERSENNE_CACHE_THRESHOLD lmmp_tune_DIVEXACT_MERSENNE_CACHE_THRESHOLD
#undef BINVERT_UNBALANCE_MODM_THRESHOLD
#define BINVERT_UNBALANCE_MODM_THRESHOLD lmmp_tune_BINVERT_UNBALANCE_MODM_THRESHOLD
#undef BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD
#define BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD lmmp_tune_BINVERT_UNBALANCE_MODM_CACHE_THRESHOLD
#undef MULHI_MERSENNE_THRESHOLD
#define MULHI_MERSENNE_THRESHOLD lmmp_tune_MULHI_MERSENNE_THRESHOLD
#undef BINVERT_UNBALANCE_SLOPE_THRESHOLD
#define BINVERT_UNBALANCE_SLOPE_THRESHOLD lmmp_tune_BINVERT_UNBALANCE_SLOPE_THRESHOLD
#undef SQRT_INVNEWTON_K_THRESHOLD
#define SQRT_INVNEWTON_K_THRESHOLD lmmp_tune_SQRT_INVNEWTON_K_THRESHOLD
#undef CBRT_INVNEWTON_EXP_SCALE
#define CBRT_INVNEWTON_EXP_SCALE lmmp_tune_CBRT_INVNEWTON_EXP_SCALE
#undef CBRT_INVNEWTON_MIN_ROOT
#define CBRT_INVNEWTON_MIN_ROOT lmmp_tune_CBRT_INVNEWTON_MIN_ROOT
#undef SQRT_NEWTON_MODM_THRESHOLD
#define SQRT_NEWTON_MODM_THRESHOLD lmmp_tune_SQRT_NEWTON_MODM_THRESHOLD
#undef CBRT_NEWTON_MODM_THRESHOLD
#define CBRT_NEWTON_MODM_THRESHOLD lmmp_tune_CBRT_NEWTON_MODM_THRESHOLD
#endif

#endif

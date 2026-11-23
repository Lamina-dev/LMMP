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

#define LMMP_TUNE_PARAMS_SOURCE
#include "tune/params.h"

#include "mparam.h"

LMMP_API mp_size_t lmmp_tune_MAT22_MUL_STRASSEN_THRESHOLD = MAT22_MUL_STRASSEN_THRESHOLD;
LMMP_API mp_size_t lmmp_tune_MUL_TOOM22_THRESHOLD = MUL_TOOM22_THRESHOLD;
LMMP_API mp_size_t lmmp_tune_SQR_TOOM22_THRESHOLD = SQR_TOOM22_THRESHOLD;
LMMP_API mp_size_t lmmp_tune_MULLO_BASECASE_THRESHOLD = MULLO_BASECASE_THRESHOLD;
LMMP_API mp_size_t lmmp_tune_SQRLO_BASECASE_THRESHOLD = SQRLO_BASECASE_THRESHOLD;
LMMP_API mp_size_t lmmp_tune_MUL_FFT_THRESHOLD = MUL_FFT_THRESHOLD;
LMMP_API mp_size_t lmmp_tune_SQR_FFT_THRESHOLD = SQR_FFT_THRESHOLD;

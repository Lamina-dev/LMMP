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

#ifndef LMMP_LMMPN_TUNE_MPARAM_H
#define LMMP_LMMPN_TUNE_MPARAM_H

#include "tune/params.h"

#if defined(LMMP_TUNE) && !defined(LMMP_TUNE_PARAMS_SOURCE)
#undef MUL_TOOM22_THRESHOLD
#define MUL_TOOM22_THRESHOLD lmmp_tune_MUL_TOOM22_THRESHOLD
#undef SQR_TOOM22_THRESHOLD
#define SQR_TOOM22_THRESHOLD lmmp_tune_SQR_TOOM22_THRESHOLD
#undef MULLO_BASECASE_THRESHOLD
#define MULLO_BASECASE_THRESHOLD lmmp_tune_MULLO_BASECASE_THRESHOLD
#undef SQRLO_BASECASE_THRESHOLD
#define SQRLO_BASECASE_THRESHOLD lmmp_tune_SQRLO_BASECASE_THRESHOLD
#undef MULLO_DC_THRESHOLD
#define MULLO_DC_THRESHOLD lmmp_tune_MULLO_DC_THRESHOLD
#undef SQRLO_DC_THRESHOLD
#define SQRLO_DC_THRESHOLD lmmp_tune_SQRLO_DC_THRESHOLD
#undef MUL_FFT_THRESHOLD
#define MUL_FFT_THRESHOLD lmmp_tune_MUL_FFT_THRESHOLD
#undef SQR_FFT_THRESHOLD
#define SQR_FFT_THRESHOLD lmmp_tune_SQR_FFT_THRESHOLD
#endif

#endif

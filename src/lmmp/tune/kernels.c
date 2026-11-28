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

#include "tune/kernels.h"

#include "impl/mul_hard.h"

LMMP_API void lmmp_tune_mul_hard_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    lmmp_mul_hard_n_(dst, numa, numb, n);
}

LMMP_API void lmmp_tune_sqr_hard_n_(mp_ptr dst, mp_srcptr numa, mp_size_t n) {
    lmmp_sqr_hard_n_(dst, numa, n);
}

LMMP_API mp_limb_t lmmp_tune_cbrt_newton_hits;
LMMP_API mp_limb_t lmmp_tune_combin_hits[LMMP_TUNE_COMBIN_PATHS];

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

#include "../../../include/lmmp/impl/mul_hard.h"


void (*const lmmp_mul_hard_fns_[LMMP_MUL_HARD_MAX_N + 1])(mp_ptr, mp_srcptr, mp_srcptr) = {
    NULL,
    lmmp_mul_hard_1_,
    lmmp_mul_hard_2_,
    lmmp_mul_hard_3_,
    lmmp_mul_hard_4_,
    lmmp_mul_hard_5_,
    lmmp_mul_hard_6_,
    lmmp_mul_hard_7_,
    lmmp_mul_hard_8_,
    lmmp_mul_hard_9_,
    lmmp_mul_hard_10_,
    lmmp_mul_hard_11_,
    lmmp_mul_hard_12_,
    lmmp_mul_hard_13_,
    lmmp_mul_hard_14_,
    lmmp_mul_hard_15_,
    lmmp_mul_hard_16_,
    lmmp_mul_hard_17_,
    lmmp_mul_hard_18_,
    lmmp_mul_hard_19_,
};

void (*const lmmp_sqr_hard_fns_[LMMP_MUL_HARD_MAX_N + 1])(mp_ptr, mp_srcptr) = {
    NULL,
    lmmp_sqr_hard_1_,
    lmmp_sqr_hard_2_,
    lmmp_sqr_hard_3_,
    lmmp_sqr_hard_4_,
    lmmp_sqr_hard_5_,
    lmmp_sqr_hard_6_,
    lmmp_sqr_hard_7_,
    lmmp_sqr_hard_8_,
    lmmp_sqr_hard_9_,
    lmmp_sqr_hard_10_,
    lmmp_sqr_hard_11_,
    lmmp_sqr_hard_12_,
    lmmp_sqr_hard_13_,
    lmmp_sqr_hard_14_,
    lmmp_sqr_hard_15_,
    lmmp_sqr_hard_16_,
    lmmp_sqr_hard_17_,
    lmmp_sqr_hard_18_,
    lmmp_sqr_hard_19_,
};

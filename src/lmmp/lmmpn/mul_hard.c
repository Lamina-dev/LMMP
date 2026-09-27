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

void lmmp_mul_hard_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    switch (n) {
        case 1: lmmp_mul_hard_1_(dst, numa, numb); break;
        case 2: lmmp_mul_hard_2_(dst, numa, numb); break;
        case 3: lmmp_mul_hard_3_(dst, numa, numb); break;
        case 4: lmmp_mul_hard_4_(dst, numa, numb); break;
        case 5: lmmp_mul_hard_5_(dst, numa, numb); break;
        case 6: lmmp_mul_hard_6_(dst, numa, numb); break;
        case 7: lmmp_mul_hard_7_(dst, numa, numb); break;
        case 8: lmmp_mul_hard_8_(dst, numa, numb); break;
        case 9: lmmp_mul_hard_9_(dst, numa, numb); break;
        case 10: lmmp_mul_hard_10_(dst, numa, numb); break;
        case 11: lmmp_mul_hard_11_(dst, numa, numb); break;
        case 12: lmmp_mul_hard_12_(dst, numa, numb); break;
        case 13: lmmp_mul_hard_13_(dst, numa, numb); break;
        case 14: lmmp_mul_hard_14_(dst, numa, numb); break;
        case 15: lmmp_mul_hard_15_(dst, numa, numb); break;
        case 16: lmmp_mul_hard_16_(dst, numa, numb); break;
        case 17: lmmp_mul_hard_17_(dst, numa, numb); break;
        case 18: lmmp_mul_hard_18_(dst, numa, numb); break;
        case 19: lmmp_mul_hard_19_(dst, numa, numb); break;
        default: lmmp_mul_basecase_(dst, numa, n, numb, n); break;
    }
}

void lmmp_sqr_hard_n_(mp_ptr dst, mp_srcptr numa, mp_size_t n) {
    switch (n) {
        case 1: lmmp_sqr_hard_1_(dst, numa); break;
        case 2: lmmp_sqr_hard_2_(dst, numa); break;
        case 3: lmmp_sqr_hard_3_(dst, numa); break;
        case 4: lmmp_sqr_hard_4_(dst, numa); break;
        case 5: lmmp_sqr_hard_5_(dst, numa); break;
        case 6: lmmp_sqr_hard_6_(dst, numa); break;
        case 7: lmmp_sqr_hard_7_(dst, numa); break;
        case 8: lmmp_sqr_hard_8_(dst, numa); break;
        case 9: lmmp_sqr_hard_9_(dst, numa); break;
        case 10: lmmp_sqr_hard_10_(dst, numa); break;
        case 11: lmmp_sqr_hard_11_(dst, numa); break;
        case 12: lmmp_sqr_hard_12_(dst, numa); break;
        case 13: lmmp_sqr_hard_13_(dst, numa); break;
        case 14: lmmp_sqr_hard_14_(dst, numa); break;
        case 15: lmmp_sqr_hard_15_(dst, numa); break;
        case 16: lmmp_sqr_hard_16_(dst, numa); break;
        case 17: lmmp_sqr_hard_17_(dst, numa); break;
        case 18: lmmp_sqr_hard_18_(dst, numa); break;
        case 19: lmmp_sqr_hard_19_(dst, numa); break;
        default: lmmp_sqr_basecase_(dst, numa, n); break;
    }
}

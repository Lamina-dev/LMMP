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

/* 硬编码函数调度: 按实测性能路由。
   mul: n<=19 全部走硬编码 (分块寄存器方案, 实测全面快于 basecase);
   sqr: n<=3 走硬编码(与库内 basecase 小规模分支同源), n>=4 库内 sqr_basecase
        的 addmul_2 内核仍快于当前硬编码交叉行方案, 故回落;
   超出 LMMP_MUL_HARD_MAX_N 或调优阈值放大时回落 basecase。
   消费方为 toom 递归宏(见 mul_toom22.c 等)。 */

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
        default: lmmp_sqr_basecase_(dst, numa, n); break;
    }
}

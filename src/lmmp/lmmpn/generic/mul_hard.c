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

/* 通用 C 硬编码平衡乘法/平方 (无汇编模式的回退实现)。

   n 为宏传入的编译期常量, 循环被编译器完全展开, 所有偏移化为立即数,
   与汇编版本同为"硬编码"语义。平方采用与汇编相同的三段式:
   交叉乘(每对(i,j),i<j 恰一次) -> 整体倍增 -> 对角平方折叠, 乘法量减半。 */

#include "../../../../include/lmmp/impl/mul_hard.h"

#define DEF_MUL_HARD(n_)                                                            \
    void lmmp_mul_hard_##n_##_(mp_ptr restrict dst, mp_srcptr restrict numa,         \
                                mp_srcptr restrict numb) {                           \
        mp_limb_t cl = 0;                                                            \
        mp_size_t i, j;                                                              \
        for (i = 0; i < n_; i++) {                                                   \
            __uint128_t t = (__uint128_t)numa[i] * numb[0] + cl;                     \
            dst[i] = (mp_limb_t)t;                                                   \
            cl = (mp_limb_t)(t >> 64);                                               \
        }                                                                            \
        dst[n_] = cl;                                                                \
        for (j = 1; j < n_; j++) {                                                   \
            cl = 0;                                                                  \
            for (i = 0; i < n_; i++) {                                               \
                __uint128_t t = (__uint128_t)numa[i] * numb[j] + dst[i + j] + cl;    \
                dst[i + j] = (mp_limb_t)t;                                           \
                cl = (mp_limb_t)(t >> 64);                                           \
            }                                                                        \
            dst[n_ + j] = cl;                                                        \
        }                                                                            \
    }

#define DEF_SQR_HARD(n_)                                                             \
    void lmmp_sqr_hard_##n_##_(mp_ptr restrict dst, mp_srcptr restrict numa) {       \
        mp_limb_t cl, x;                                                             \
        mp_size_t i, j;                                                              \
        /* 交叉列 0 恒为 0, 顶列 2n-1 交叉不触及 */                                   \
        dst[0] = 0;                                                                  \
        /* 行 0: dst[1..n] = a[1..n)*a_0 (交叉, 纯写) */                              \
        x = numa[0];                                                                 \
        cl = 0;                                                                      \
        for (i = 1; i < n_; i++) {                                                   \
            __uint128_t t = (__uint128_t)numa[i] * x + cl;                           \
            dst[i] = (mp_limb_t)t;                                                   \
            cl = (mp_limb_t)(t >> 64);                                               \
        }                                                                            \
        dst[n_] = cl;                                                                \
        /* 行 i: dst[i+1..i+n] += a[i+1..n)*a_i */                                   \
        for (j = 1; j + 1 < n_; j++) {                                               \
            x = numa[j];                                                             \
            cl = 0;                                                                  \
            for (i = j + 1; i < n_; i++) {                                           \
                __uint128_t t = (__uint128_t)numa[i] * x + dst[i + j] + cl;          \
                dst[i + j] = (mp_limb_t)t;                                           \
                cl = (mp_limb_t)(t >> 64);                                           \
            }                                                                        \
            dst[n_ + j] = cl;                                                        \
        }                                                                            \
        /* 倍增: dst[1..2n-2] = 2*dst, 顶列 2n-1 = 进位(交叉恒0) */                   \
        cl = 0;                                                                      \
        for (i = 1; i + 1 < 2 * n_; i++) {                                           \
            __uint128_t t = (__uint128_t)dst[i] * 2 + cl;                            \
            dst[i] = (mp_limb_t)t;                                                   \
            cl = (mp_limb_t)(t >> 64);                                               \
        }                                                                            \
        dst[2 * n_ - 1] = cl;                                                        \
        /* 对角: dst[2i] += a_i^2, 进位经奇列 2i+1 传播 */                            \
        cl = 0;                                                                      \
        for (i = 0; i < n_; i++) {                                                   \
            __uint128_t t = (__uint128_t)numa[i] * numa[i] + dst[2 * i] + cl;        \
            dst[2 * i] = (mp_limb_t)t;                                               \
            cl = (mp_limb_t)(t >> 64);                                               \
            t = (__uint128_t)dst[2 * i + 1] + cl;                                    \
            dst[2 * i + 1] = (mp_limb_t)t;                                           \
            cl = (mp_limb_t)(t >> 64);                                               \
        }                                                                            \
    }

DEF_MUL_HARD(1)
DEF_MUL_HARD(2)
DEF_MUL_HARD(3)
DEF_MUL_HARD(4)
DEF_MUL_HARD(5)
DEF_MUL_HARD(6)
DEF_MUL_HARD(7)
DEF_MUL_HARD(8)
DEF_MUL_HARD(9)
DEF_MUL_HARD(10)
DEF_MUL_HARD(11)
DEF_MUL_HARD(12)
DEF_MUL_HARD(13)
DEF_MUL_HARD(14)
DEF_MUL_HARD(15)
DEF_MUL_HARD(16)
DEF_MUL_HARD(17)
DEF_MUL_HARD(18)
DEF_MUL_HARD(19)

DEF_SQR_HARD(1)
DEF_SQR_HARD(2)
DEF_SQR_HARD(3)
DEF_SQR_HARD(4)
DEF_SQR_HARD(5)
DEF_SQR_HARD(6)
DEF_SQR_HARD(7)
DEF_SQR_HARD(8)
DEF_SQR_HARD(9)
DEF_SQR_HARD(10)
DEF_SQR_HARD(11)
DEF_SQR_HARD(12)
DEF_SQR_HARD(13)
DEF_SQR_HARD(14)
DEF_SQR_HARD(15)
DEF_SQR_HARD(16)
DEF_SQR_HARD(17)
DEF_SQR_HARD(18)
DEF_SQR_HARD(19)

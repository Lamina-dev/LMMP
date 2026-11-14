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

#ifndef __LMMP_IMPL_MUL_HARD_H__
#define __LMMP_IMPL_MUL_HARD_H__

#include "../lmmpn.h"


#define LMMP_MUL_HARD_MAX_N 19

#define LMMP_DECL_MUL_HARD(n) \
    void lmmp_mul_hard_##n##_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb);
#define LMMP_DECL_SQR_HARD(n) void lmmp_sqr_hard_##n##_(mp_ptr dst, mp_srcptr numa);

LMMP_DECL_MUL_HARD(1)
LMMP_DECL_MUL_HARD(2)
LMMP_DECL_MUL_HARD(3)
LMMP_DECL_MUL_HARD(4)
LMMP_DECL_MUL_HARD(5)
LMMP_DECL_MUL_HARD(6)
LMMP_DECL_MUL_HARD(7)
LMMP_DECL_MUL_HARD(8)
LMMP_DECL_MUL_HARD(9)
LMMP_DECL_MUL_HARD(10)
LMMP_DECL_MUL_HARD(11)
LMMP_DECL_MUL_HARD(12)
LMMP_DECL_MUL_HARD(13)
LMMP_DECL_MUL_HARD(14)
LMMP_DECL_MUL_HARD(15)
LMMP_DECL_MUL_HARD(16)
LMMP_DECL_MUL_HARD(17)
LMMP_DECL_MUL_HARD(18)
LMMP_DECL_MUL_HARD(19)

LMMP_DECL_SQR_HARD(1)
LMMP_DECL_SQR_HARD(2)
LMMP_DECL_SQR_HARD(3)
LMMP_DECL_SQR_HARD(4)
LMMP_DECL_SQR_HARD(5)
LMMP_DECL_SQR_HARD(6)
LMMP_DECL_SQR_HARD(7)
LMMP_DECL_SQR_HARD(8)
LMMP_DECL_SQR_HARD(9)
LMMP_DECL_SQR_HARD(10)
LMMP_DECL_SQR_HARD(11)
LMMP_DECL_SQR_HARD(12)
LMMP_DECL_SQR_HARD(13)
LMMP_DECL_SQR_HARD(14)
LMMP_DECL_SQR_HARD(15)
LMMP_DECL_SQR_HARD(16)
LMMP_DECL_SQR_HARD(17)
LMMP_DECL_SQR_HARD(18)
LMMP_DECL_SQR_HARD(19)

/**
 * @brief 硬编码平衡乘法跳转表, [i] 即 lmmp_mul_hard_i_, 下标 0 为 NULL
 * @note 调用点原地取函数指针直接调用(单次 call 进入目标), 省去经分派函数
 *        中转的又一次 call; 按短乘数长度固化调用的场景(如不平衡乘法分块
 *        累加)可将指针外提出循环; 下标必须满足 0 < i <= LMMP_MUL_HARD_MAX_N
 */
extern void (*const lmmp_mul_hard_fns_[LMMP_MUL_HARD_MAX_N + 1])(mp_ptr dst, mp_srcptr numa, mp_srcptr numb);

/**
 * @brief 硬编码平衡平方跳转表, [i] 即 lmmp_sqr_hard_i_, 下标 0 为 NULL
 * @note 同 lmmp_mul_hard_fns_; 下标必须满足 0 < i <= LMMP_MUL_HARD_MAX_N
 */
extern void (*const lmmp_sqr_hard_fns_[LMMP_MUL_HARD_MAX_N + 1])(mp_ptr dst, mp_srcptr numa);

/**
 * @brief 硬编码平衡乘法分派 [dst,2n] = [numa,n] * [numb,n]
 * @param dst 结果输出指针
 * @param numa 第一个乘数
 * @param numb 第二个乘数
 * @param n 操作数 limb 长度
 * @warning 0 < n, sep(dst,[numa|numb]), dst!=NULL, numa!=NULL, numb!=NULL;
 *          n <= LMMP_MUL_HARD_MAX_N 时查表直调硬编码函数, 其余回落 mul_basecase
 * @return 无
 */
static inline void __lmmp_mul_hard_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    if (n <= LMMP_MUL_HARD_MAX_N)
        lmmp_mul_hard_fns_[n](dst, numa, numb);
    else
        lmmp_mul_basecase_(dst, numa, n, numb, n);
}

/**
 * @brief 硬编码平衡平方分派 [dst,2n] = [numa,n]^2
 * @param dst 结果输出指针
 * @param numa 输入操作数
 * @param n 操作数 limb 长度
 * @warning 0 < n, sep(dst,numa), dst!=NULL, numa!=NULL;
 *          n <= LMMP_MUL_HARD_MAX_N 时查表直调硬编码函数, 其余回落 sqr_basecase
 * @return 无
 */
static inline void __lmmp_sqr_hard_n_(mp_ptr dst, mp_srcptr numa, mp_size_t n) {
    if (n <= LMMP_MUL_HARD_MAX_N)
        lmmp_sqr_hard_fns_[n](dst, numa);
    else
        lmmp_sqr_basecase_(dst, numa, n);
}

#define lmmp_mul_hard_n_ __lmmp_mul_hard_n_
#define lmmp_sqr_hard_n_ __lmmp_sqr_hard_n_

#endif /* __LMMP_IMPL_MUL_HARD_H__ */

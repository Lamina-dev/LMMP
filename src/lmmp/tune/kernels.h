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

#ifndef LMMP_TUNE_KERNELS_H
#define LMMP_TUNE_KERNELS_H

#include "lmmp/lmmp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 硬编码平衡乘法分派的调优薄封装 [dst,2n] = [numa,n] * [numb,n]
 * @warning 0 < n, sep(dst,[numa|numb]); n <= LMMP_MUL_HARD_MAX_N 查表直调, 其余回落 mul_basecase
 * @note hard 函数表是数据符号, Windows 下不随 DLL 自动导出, 故在 tune 库内封装供程序直调真实低路。
 */
LMMP_API void lmmp_tune_mul_hard_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);

/**
 * @brief 硬编码平衡平方分派的调优薄封装 [dst,2n] = [numa,n]^2
 * @warning 0 < n, sep(dst,numa); n <= LMMP_MUL_HARD_MAX_N 查表直调, 其余回落 sqr_basecase
 * @note 同 lmmp_tune_mul_hard_n_。
 */
LMMP_API void lmmp_tune_sqr_hard_n_(mp_ptr dst, mp_srcptr numa, mp_size_t n);

/** @brief lmmp_cbrt_ 进入牛顿分支的累计次数（仅 tune 构建导出）。 */
LMMP_API extern mp_limb_t lmmp_tune_cbrt_newton_hits;

#ifdef __cplusplus
}
#endif

#endif

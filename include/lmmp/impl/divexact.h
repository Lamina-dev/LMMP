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

#ifndef __LMMP_DIVEXACT_H__
#define __LMMP_DIVEXACT_H__

#include "../lmmpn.h"

/**
 * @brief 精确除以3（[dst,na] = [numa,na] / 3）
 * @param dst 结果存储位置
 * @param numa 被除数
 * @param na 被除数长度
 * @warning eqsep(dst,numa), na>0, dst!=NULL, numa!=NULL
 */
void lmmp_divexact_by3_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 精确除以9（[dst,na] = [numa,na] / 9）
 * @param dst 结果存储位置
 * @param numa 被除数
 * @param na 被除数长度
 * @warning eqsep(dst,numa), na>0, dst!=NULL, numa!=NULL
 */
void lmmp_divexact_by9_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 精确除以15（[dst,na] = [numa,na] / 15）
 * @param dst 结果存储位置
 * @param numa 被除数
 * @param na 被除数长度
 * @warning eqsep(dst,numa), na>0, dst!=NULL, numa!=NULL
 */
void lmmp_divexact_by15_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

/**
 * @brief 通用奇除数精确除，可带 2^shr 因子：
 *        [dst,na] = [numa,na] / (d*2^shr)，其中 d 为奇数、di = d^(-1) mod B
 * @param dst 结果存储位置
 * @param numa 被除数
 * @param na 被除数长度
 * @param d 奇除数（单 limb）
 * @param di d 模 B 的乘法逆元
 * @param shr 2 的幂因子指数（0 <= shr < LIMB_BITS）
 * @return 残差（精确除时为 0）
 * @warning eqsep(dst,numa), na>0, d 为奇数, d*di == 1 (mod B)
 * @note 2^shr 因子按"被除数位流右移 shr 位后除奇部 d"处理，结果仅在
 *       低 64*na-shr 位上有定义（顶部 shr 位不定）
 */
mp_limb_t lmmp_divexact_odd_pi_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t d, mp_limb_t di, unsigned shr);

#endif // __LMMP_DIVEXACT_H__

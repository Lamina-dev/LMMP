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

#endif // __LMMP_DIVEXACT_H__

/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#ifndef __LMMP_LOG2_EXP2_H__
#define __LMMP_LOG2_EXP2_H__

#include <stdint.h>

/**
 * @brief floor(log2(1+x/B)*B), B=2^64
 * @param x 输入的小数部分
 * @return floor(log2(1+x/B)*B)
 */
uint64_t log2_fixed_64(uint64_t x);

/**
 * @brief floor(exp2(x/B)*B-B), B=2^64
 * @param x 输入的小数部分
 * @return floor(exp2(x/B)*B-B)
 */
uint64_t exp2_fixed_64(uint64_t x);

#if 0
/**
 * @brief floor(log2(1+x/B)*B), B=2^128
 * @param high 输入的小数部分高64位
 * @param low 输入的小数部分低64位
 * @param dst 输出数组（2个元素）：dst[0] 为低64位，dst[1] 为高64位
 * @note x = high * 2^64 + low
 */
void log2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low);

/**
 * @brief floor(exp2(x/B)*B-B), B=2^128
 * @param high 输入的小数部分高64位
 * @param low 输入的小数部分低64位
 * @param dst 输出数组（2个元素）：dst[0] 为低64位，dst[1] 为高64位
 * @note x = high * 2^64 + low
 */
void exp2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low);
#endif // 0

#endif // __LMMP_LOG2_EXP2_H__
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

#ifndef __LMMP_LOG2_EXP2_H__
#define __LMMP_LOG2_EXP2_H__

#include <stdint.h>

/**
 * @brief 计算 floor(log2(1+x/B)*B), B=2^64
 * @param x 输入的小数部分
 * @return floor(log2(1+x/B)*B) + e，|e| <= 2
 * @note 表驱动 + 短泰勒级数实现，返回契约允许 +-2 ulp 的误差
 *       （实测 |e| <= 2，主要来自各级 floor 舍入）。主要供 cbrt/nthroot
 *       等内部使用，配合外部单调修正即可得到精确整值结果。
 */
uint64_t log2_fixed_64(uint64_t x);

/**
 * @brief 计算 floor(exp2(x/B)*B-B), B=2^64
 * @param x 输入的小数部分
 * @return floor(exp2(x/B)*B-B) + e，|e| <= 2
 * @note 同 log2_fixed_64，返回契约允许 +-2 ulp 的误差。
 */
uint64_t exp2_fixed_64(uint64_t x);

/**
 * @brief 计算 floor(log2(1+x/B)*B), B=2^128
 * @param high 输入的小数部分高64位
 * @param low 输入的小数部分低64位
 * @param dst 输出数组（2个元素）：dst[0] 为低64位，dst[1] 为高64位
 * @note x = high * 2^64 + low，结果误差 |e| <= 2（128bit ulp）
 */
void log2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low);

/**
 * @brief 计算 floor(exp2(x/B)*B-B), B=2^128
 * @param high 输入的小数部分高64位
 * @param low 输入的小数部分低64位
 * @param dst 输出数组（2个元素）：dst[0] 为低64位，dst[1] 为高64位
 * @note x = high * 2^64 + low，结果误差 |e| <= 2（128bit ulp）
 */
void exp2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low);

#endif // __LMMP_LOG2_EXP2_H__
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

#ifndef __LMMP_LGLG_H__
#define __LMMP_LGLG_H__

#include "longlong.h"
#include "../lmmp.h"
#include <stdint.h>


// 索引i处对应的数值为：round(log2(1+(i+1)/512) * 2^32) 
// 除了最后一个元素（最后一个即log2(2)*2^32，受限于32位整数，取为2^32-1）
extern const uint32_t log2_fix32_q9[512];

#define tab(i) log2_fix32_q9[i - 1]
#define H 0x400000 // 插值点距离
#define adj_H 0x200000

/*
  为了不让你读这里的代码时，发出fuck的声音，我写下这段注释帮助你理解。

  首先，主要有两类计算：
  1. 计算x*log2(n)的ceil值
  2. 计算log2(gamma(n))的ceil和floor值
  由于是用于估计64位字的缓冲区长度。所以只需要上界不低估，下界不高估即可。

  log2_fix32_q9 数组，存储了log2(1+(i+1)/512) * 2^32的值，其中i从0到511。
  除了最后一位，我们向下舍入到0xffffffff，因为log2(1+(511+1)/512) * 2^32
  的实际值为0x100000000。我们假设要计算log2(n)，我们假定已经把n移位至MSB(n)=1
  我们使用log2_fix32_q9[idx]来进行线性插值，当然为了保证不低估，我们采用割线插值。
  n的形式为：1.xxxxxxxxx... * 2^31 其中我们根据xxx段（9bit），匹配到log2_fix32_q9[idx]。
  找到临近的插值点，靠近左边，则使用左边两个插值点进行插值（由于log2(x)是凹函数，
  在插值点右边的n一定大于实际值）；靠近右边，则使用右边两个插值点进行插值。同时，
  我们对两端边界情况也进行了处理。

  对于gamma函数，我们使用斯特林公式 gamma(x) ~ sqrt(2pi*x)*(x/e)^x
  log2(gamma(x)) ~ (x+1/2)*log2(x) - x*log2(e) + log2(sqrt(2pi))
  由于斯特林公式产生的相对误差是随x的增大而减小，且我们最终需要取整，尾部的常数
  我们直接忽略即可。
  实际计算式为：log2(gamma(x)) = {(2x+1)*log2(x) - 2*[x + x*(log2(e)-1)] + log2(2pi)} / 2

  中间变量最大值产生在(2x+1)*log2(x)，其中log2(x)最多为5bit，乘2需要1个bit，同时，
  由于插值公式本身就存在误差，所以中间计算时，我们只需要保留26个bit的信息即可。
*/

/**
 * @brief 计算x*log2(n)的ceil值
 * @param n 底数
 * @param x 乘数
 * @warning MSB(n)=1
 * @note 大约产生10^-7相对误差
 * @return n*log2(x)的ceil值
 */
static inline uint64_t xlog2n_ceil(uint32_t x, uint32_t n) {
    /*
    此函数虽然大部分计算和log2n_2n1_ceil相同，但后者由于有其他函数配合误差控制，
    在这个函数中，为了保证不低估，我们使用了更加严格的计算，比如使用adj_H来调控
    除以H时的除法误差，并且末尾的调整也更加严格。
    */
    n &= 0x7fffffff;
    uint32_t idx = n >> 22;

    uint32_t idx1 = (idx) << 22;
    uint32_t idx2 = (idx + 1) << 22;

    uint64_t r;
    uint64_t x64 = (uint64_t)x;
    if ((idx != 511 && 2 * n >= (idx1 + idx2)) || idx == 0 || idx == 1) {
        // 使用右边两个插值点进行插值
        uint64_t y1 = tab(idx + 1);
        uint64_t y2 = tab(idx + 2);
        idx1 = (idx + 1) << 22;
        idx2 = (idx + 2) << 22;
        int32_t x2x = idx2 - n;
        int32_t x1x = idx1 - n;
        r = (y1 * x2x - y2 * x1x);
        r += adj_H;
        r /= H;
    } else {
        // 使用左边两个插值点进行插值
        uint64_t y1 = tab(idx - 1);
        uint64_t y2 = tab(idx);
        idx1 = (idx - 1) << 22;
        idx2 = (idx) << 22;
        int32_t x2x = n - idx2;
        int32_t x1x = n - idx1;
        r = (y2 * x1x - y1 * x2x);
        r += adj_H;
        r /= H;
        // 这是插值结果可能高过log2(2)，我们直接截断数据
        r = (r >= 0x100000000) ? 0x100000000 : r;
    }
    r *= x;
    r >>= 6;
    x64 *= 31;
    x64 <<= 26;
    r += x64;
    int adj = (r & 0x3FFFFFF) ? 2 : 1;
    return (r >> 26) + adj;
}

/**
 * @brief 计算(2n+1)*log2(n)的ceil值
 * @param n 底数
 * @warning n>1
 * @return (2n+1)*log2(n)的定点数格式，低位26位为小数
 */
static inline uint64_t log2n_2n1_ceil(uint32_t n) {
    uint32_t x = n;
    int msb;
    clz_shl_u32(n, n, msb);
    n &= 0x7fffffff;
    uint32_t idx = n >> 22;

    uint32_t idx1 = (idx) << 22;
    uint32_t idx2 = (idx + 1) << 22;

    uint64_t r, r2;
    if ((idx != 511 && 2 * n >= (idx1 + idx2)) || idx == 0 || idx == 1) {
        // 使用右边两个插值点进行插值
        uint64_t y1 = tab(idx + 1);
        uint64_t y2 = tab(idx + 2);
        idx1 = (idx + 1) << 22;
        idx2 = (idx + 2) << 22;
        int32_t x2x = idx2 - n;
        int32_t x1x = idx1 - n;

        r = (y1 * x2x - y2 * x1x);
        r /= H;
    } else {
        // 使用左边两个插值点进行插值
        uint64_t y1 = tab(idx - 1);
        uint64_t y2 = tab(idx);
        idx1 = (idx - 1) << 22;
        idx2 = (idx) << 22;
        int32_t x2x = n - idx2;
        int32_t x1x = n - idx1;

        r = (y2 * x1x - y1 * x2x);
        r /= H;
        r = (r >= 0x100000000) ? 0xffffffff : r;
    }
    /*
    n = x = n' * 2^k，k = 31 - msb
    我们要计算的是 (2n+1)log2(n) 即 (2x+1)log2(n') + (2x+1)*k
    */
    r2 = r >> 6;
    r *= x;      
    r >>= 6;
    uint64_t ret = 2 * r + r2;
    uint64_t x64 = (uint64_t)x * 2 + 1;
    x64 *= (31 - msb);
    x64 <<= 26;
    return ret + x64;
}

/**
 * @brief 计算(2n+1)*log2(n)的floor值
 * @param n 底数
 * @warning n>1
 * @return (2n+1)*log2(n)的定点数格式，低位26位为小数
 */
static inline uint64_t log2n_2n1_floor(uint32_t n) {
    uint32_t x = n;
    int msb;
    clz_shl_u32(n, n, msb);
    n &= 0x7fffffff;
    uint32_t idx = n >> 22;

    uint32_t idx1 = (idx) << 22;
    // 直接取相邻两点插值，此时，插值线性函数必定在log2(x)下方
    uint64_t r, r2, y1, y2;
    if (idx == 0) {
        // 索引0对应的值即log2(1+(0)/512) * 2^32 = 0
        y1 = 0;
        y2 = tab(1);
    } else {
        y1 = tab(idx);
        y2 = tab(idx + 1);
        y2 -= y1;
    }
    uint32_t x1x = n - idx1;
    r = y2 * x1x / H;
    r += y1;
    /*
    n = x = n' * 2^k，k = 31 - msb
    我们要计算的是 (2n+1)log2(n) 即 (2x+1)log2(n') + (2x+1)*k
    */
    r2 = r >> 6;
    r *= x;
    r >>= 6;
    uint64_t ret = 2 * r + r2;
    uint64_t x64 = (uint64_t)x * 2 + 1;
    x64 *= (31 - msb);
    x64 <<= 26;
    return ret + x64;
}
#undef tab
#undef H
#undef adj_H

/**
 * @brief 计算 n * (log2(e)-1)
 * @return n * (log2(e)-1)的定点数格式，低位32位为小数
 */
static inline uint64_t mul_log2e_1_ceil(uint32_t n) {
    uint64_t r = 1901360723;  // ceil((log2(e)-1)*2^32)
    r *= n;
    return r;
}

/**
 * @brief 计算 n * (log2(e)-1)
 * @return n * (log2(e)-1)的定点数格式，低位32位为小数
 */
static inline uint64_t mul_log2e_1_floor(uint32_t n) {
    uint64_t r = 1901360722;  // floor((log2(e)-1)*2^32)
    r *= n;
    return r;
}

/**
 * @brief 计算 log2(n!)的ceil值
 * @param n 底数
 * @warning n > 2
 * @return log2(n!)的ceil值
 */
static inline uint64_t log2_fac_ceil(uint32_t n) {
    uint64_t r3 = (uint64_t)n << 26;
    uint64_t r4 = mul_log2e_1_floor(n);

    r4 >>= 6;
    uint64_t r = log2n_2n1_ceil(n);
    r -= (r3 + r4) * 2;
    r += 177938894;  // 177938894 = ceil(log2(2pi)*2^26)
    int adj = (r & 0x3FFFFFF) ? 1 : 0;
    r >>= 26;
    return (1 + r) / 2 + adj;
}

/**
 * @brief 计算 log2(n!)的floor值
 * @param n 底数
 * @warning n > 2
 * @return log2(n!)的floor值
 */
static inline uint64_t log2_fac_floor(uint32_t n) {
    uint64_t r3 = (uint64_t)n << 26;
    uint64_t r4 = mul_log2e_1_ceil(n);

    r4 >>= 6;
    uint64_t r = log2n_2n1_floor(n);
    r -= (r3 + r4) * 2;
    r += 177938893; // 177938893 = floor(log2(2pi)*2^26)
    r >>= 26;
    return r / 2;
}

#endif // __LMMP_LGLG_H__
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

#ifndef __LMMP_PRIME_TABLE_H__
#define __LMMP_PRIME_TABLE_H__

#include "../numth.h"
#include <math.h>

typedef uint64_t lmmp_bitset_t;
typedef uint64_t* lmmp_bitset_p;

#define LMMP_BITSET_BITS (64)
#define LMMP_BITSET_MASK (0xffffffffffffffffull)
#define LMMP_BITSET_BYTES (8)

#define PRIME_SHORT_TABLE_SIZE 6542

#define PRIME_SHORT_TABLE_N 0x10000

extern const ushort prime_short_table[PRIME_SHORT_TABLE_SIZE];

/**
 * @brief 根据全局素数表判断一个数是否为素数
 * @param p 待判断的数
 * @warning p>1, p%2==1
 * @note 若 p 超过了当前全局素数表的范围，则返回 -1
 * @return 1 素数，0 合数，-1 超出质数表范围
 */
int lmmp_is_prime_table_(uint p);

/**
 * @brief 计算小于等于 n 的素数数量
 * @param n 范围
 * @return 素数数量
 */
ushort lmmp_prime_cnt16_(ushort n);

/**
 * @brief 估计 n 范围内的素数数量
 * @param n 范围
 * @note 不会低估素数数量，可能恰好超过 pi(n)，用以估计素数数组需要的空间
 * @return 素数数量
 */
static inline ulong lmmp_prime_size_(ulong n) {
    if (n < PRIME_SHORT_TABLE_N) {
        return lmmp_prime_cnt16_(n);
    } else if (n < 95000) {
        return (ulong)ceil((double)n / (log(n) - 1.095)) + 1;
    } else if (n < 355991) {
        return (ulong)ceil((double)n / (log(n) - 1.0975)) + 1;
    } else if (n < 1332479531) {
        // Dusart 2000 估计 - π(x)的上界
        // lmmp_debug_assert(n >= 355991);
        double x = (double)n;
        double lnx = log(x);
        double lnx2 = lnx * lnx;
        double r = x / lnx * (1.0 + 1.0 / lnx + 2.51 / lnx2);
        return (ulong)r;
    }
    /*
         Dusart 2010 估计 - π(x)的严格上界
         from: Dusart (2010) "Estimates of some functions over primes without R.H."(https://arxiv.org/abs/1002.0442)
         U(x) = x / [lnx
                     - 1
                     - 1/lnx
                     - 3.35/(lnx)^2
                     - 12.65/(lnx)^3
                     - 71.7/(lnx)^4
                     - 466.1275/(lnx)^5
                     - 3489.8225/(lnx)^6]
    */
    // lmmp_debug_assert(n >= 1332479531);
    double lnx = log(n);
    double lnx2 = lnx * lnx;
    double lnx3 = lnx2 * lnx;
    double lnx4 = lnx3 * lnx;
    double lnx5 = lnx4 * lnx;
    double lnx6 = lnx5 * lnx;

    double denom =
        lnx - 1.0 - 1.0 / lnx - 3.35 / lnx2 - 12.65 / lnx3 - 71.7 / lnx4 - 466.1275 / lnx5 - 3489.8225 / lnx6;

    return (ulong)ceil(n / denom);
}

/**
 * @brief 初始化全局素数表
 * @param n 素数表大小（含n）
 * @warning 会自动扩容，可以重入；当 n < PRIME_SHORT_TABLE_N 时，将会直接返回
 * @note 仅存储奇素数
 */
void lmmp_prime_int_table_init_(uint n);

/**
 * @brief 释放全局素数表
 */
void lmmp_prime_int_table_free_(void);

typedef struct {
    uintp restrict pp; // 质数数组（升序排列）
    uint size;         // pp 数组大小
    uint start_idx;    // 位图下一次解析的起始索引（字）
    uint end_idx;      // 位图应该终止解析的结束索引（字）
    uint end_num;      // 终止解析的最大数
    int is_end;        // 是否已经遍历到全局质数表末尾
} prime_cache_t;

/*
 示例代码（遍历全局奇素数表）：

    lmmp_prime_int_table_init_(n);
    prime_cache_t cache;
    lmmp_prime_cache_init_(&cache, n);
    while (cache.is_end == 0) {
        lmmp_prime_cache_next_(&cache);
        for (uint i = 0; i < cache.size; i++) {
            ....
        }
    }
    lmmp_prime_cache_free_(&cache);
*/

/**
 * @brief 初始化素数表缓存
 * @param cache 缓存结构体
 * @param n 遍历素数表的范围（超过n时或者遍历到全局质数表末尾，is_end 置为 1）
 */
void lmmp_prime_cache_init_(prime_cache_t* cache, uint n);

/**
 * @brief 素数表缓存更新（从小到大遍历全局质数表）
 * @param cache 缓存结构体
 * @note 缓存只存储奇素数，当遍历到全局质数表末尾或者达到设置的范围时，is_end 置为 1
 */
void lmmp_prime_cache_next_(prime_cache_t* cache);

/**
 * @brief 释放素数表缓存
 * @param cache 缓存结构体
 */
void lmmp_prime_cache_free_(prime_cache_t* cache);

// 3,5,7,11的余数掩码表
extern const lmmp_bitset_t r35711_mask_map[19];

/**
 * @brief 校验是否能被3,5,7,11整除，能够整除则返回1，否则返回0
 */
static inline int trial_div35711(ulong n) {
    uint rem = n % 1155; // 1155 = 3 * 5 * 7 * 11
    uint idx = rem / LMMP_BITSET_BITS;
    uint bit = rem % LMMP_BITSET_BITS;
    return (r35711_mask_map[idx] >> bit) & 1ULL;
}

#endif  // __LMMP_PRIME_TABLE_H__
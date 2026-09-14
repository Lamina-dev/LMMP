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

#ifndef __LMMP_STR_CONV_H__
#define __LMMP_STR_CONV_H__

#include "../lmmpn.h"
#include "inlines.h"

// 十进制特化转换的基数
#define DEC_BASE 10

typedef struct mp_base_t {
    // 单个limb能容纳的基数的最大幂次
    // 二的幂次存储 log2(base)
    // large_base = base ^ digits_in_limb
    mp_limb_t large_base;
    // ceiling(2^64*log2(base)/log2(2^64))
    // N 位 base 数最多需要 N * lg_base / 2^64 + 1 个 limb
    mp_limb_t lg_base;
    // N 位二进制数最多需要 N * inv_lg_base / 2^64 + 1 个 base 进制位
    // ceiling(2^64/log2(base))
    mp_limb_t inv_lg_base;
    // 单个limb可容纳的最大基数位数
    // floor(64/log2(base))
    int digits_in_limb;
    // 基数（2~256）
    int base;
} mp_base_t;

typedef struct mp_basepow_t {
    // 基数幂值(base^digits)
    mp_ptr p;
    // p的 limb 长度
    mp_size_t np;
    // 归一化p的逆元
    mp_ptr invp;
    // invp的有效长度
    mp_size_t ni;
    // 去除的末尾零 limb 长度
    mp_size_t zeros;
    // 基数幂的指数（log_base(p)）
    mp_size_t digits;
    // p归一化时的移位位数
    int norm_cnt;
    // 基数
    int base;
} mp_basepow_t;

extern const mp_base_t lmmp_bases_table[255];

/**
 * @brief basecase 字节串转 limb 数组引擎
 * @param dst 结果输出指针
 * @param src 数字（或字符）数组源指针
 * @param len 数组长度
 * @param base 进制基数
 * @param off 数字字节偏移量（0 或 '0'）
 * @warning len>0, src[len-1]!=off, dst!=NULL, src!=NULL
 * @return 转换后的结果的 limb 长度
 */
mp_size_t lmmp_from_str_basecase_(mp_ptr dst, const mp_byte_t* src, mp_size_t len, int base, mp_byte_t off);

/**
 * @brief 分治字节串转 limb 数组引擎
 * @param dst 结果输出指针
 * @param src 数字（或字符）数组源指针
 * @param len 数组长度
 * @param pow 指数表
 * @param tp 临时数组
 * @param off 数字字节偏移量（0 或 '0'）
 * @warning src[len-1]!=off, sep(dst,tp), dst!=NULL, src!=NULL, tp!=NULL, len>0
 * @note 第一层调用时：nh>=2, [dst,2*N], [tp,limbs]
 *       后序递归时：N>=2, [dst,limbs+1], [tp,2*N-1]
 *       limbs为返回值，N = pow->np + pow->zeros
 * @return 转换后的 limb 数量
 */
mp_size_t lmmp_from_str_divide_(mp_ptr dst, const mp_byte_t* src, mp_size_t len, mp_basepow_t* pow, mp_ptr tp, mp_byte_t off);

/**
 * @brief basecase limb 数组转字节串引擎
 * @param dst 结果输出指针
 * @param numa 输入指针
 * @param na 输入的 limb 长度
 * @param base 目标进制基数
 * @param off 数字字节偏移量（0 或 '0'）
 * @warning numa[na-1]!=0, dst!=NULL, numa!=NULL
 * @return 转换后的字节串长度
 */
mp_size_t lmmp_to_str_basecase_(mp_byte_t* dst, mp_srcptr numa, mp_size_t na, int base, mp_byte_t off);

/**
 * @brief 分治 limb 数组转字节串引擎
 * @param dst 结果输出指针
 * @param numa 输入指针（运算后损坏）
 * @param na 输入的 limb 长度
 * @param pow 指数表
 * @param tpq 临时数组
 * @param off 数字字节偏移量（0 或 '0'）
 * @warning numa[na-1]!=0, sep(dst,tpq), dst!=NULL, numa!=NULL, tpq!=NULL, pow!=NULL
 * @return 转换后的字节串长度
 */
mp_size_t lmmp_to_str_divide_(mp_byte_t* dst, mp_ptr numa, mp_size_t na, mp_basepow_t* pow, mp_ptr tpq, mp_byte_t off);

/**
 * @brief 获取（惰性构建/扩展）全局十进制幂表
 * @param grps 所需覆盖的最大 19 位组数（>1）
 * @param ctop 输出最高有效层索引
 * @return 幂表数组 [0..*ctop]，pow[k] = 10^(19*2^k)
 * @warning grps>1, ctop!=NULL, 需已调用 lmmp_global_init
 * @note 幂值已归一化并按需预计算逆元；*ctop = floor(log2(grps))；
 *       表按需增长（仅新增层，已有层不变），lmmp_global_deinit 时释放
 */
mp_basepow_t* lmmp_dec_pow_table_(mp_size_t grps, int* ctop);

/**
 * @brief 释放全局十进制幂表（可重入）
 */
void lmmp_dec_pow_table_free_(void);

#endif // __LMMP_STR_CONV_H__
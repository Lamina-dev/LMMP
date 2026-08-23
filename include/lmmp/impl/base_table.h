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

#ifndef __LMMP_BASE_TABLE_H__
#define __LMMP_BASE_TABLE_H__

#include "../lmmpn.h"

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

#endif // __LMMP_BASE_TABLE_H__
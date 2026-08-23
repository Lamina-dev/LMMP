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

#ifndef LMMP_SECRET_H
#define LMMP_SECRET_H

/*
  LMMP中实现了两种hash函数：SipHash-2-4 和 xxhash。其中前者通常被认为安全性更好，被认为可以抵挡hash洪水攻击，而后者
  提供更快的速度，测量发现：两者生成的 hash 值均具有非常良好的统计性能，同时生成速度 xxhash 比 SipHash-2-4 快四倍左右。

  需要注意的是，两种hash函数都不是标准处理任意字节流的 hash 函数，因此在 LMMP 中，它们仅用于对整数数据进行 hash 计算，
  尽管这可能带来未知的安全风险，但如果仅作为hash表的键值，则影响不大。但对于字节流数据，应使用标准的 hash 函数，我们建议
  使用其他更强的加密算法或协议来处理。
 */

#include "lmmp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const uint64_t srckey64_t[1];
typedef const uint64_t srckey128_t[2];
typedef const uint64_t srckey256_t[4];

typedef uint64_t key64_t[1];
typedef uint64_t key128_t[2];
typedef uint64_t key256_t[4];


/**
 * @brief SipHash-2-4 函数（非标准处理任意字节流的 SipHash-2-4）
 * @param in 输入数据，可以为 NULL
 * @param inlen 输入数据长度
 * @param key 128-bit 秘钥，可以为 NULL
 * @warning 若 key 为 NULL，则使用全零秘钥
 * @return 64-bit hash 值
 */
LMMP_API uint64_t lmmp_siphash24_(mp_srcptr in, mp_size_t inlen, srckey128_t key);

/**
 * @brief xxhash 函数（非标准处理任意字节流的 xxhash）
 * @param in 输入数据，可以为 NULL
 * @param inlen 输入数据长度
 * @param key 64-bit 秘钥，可以为 NULL
 * @warning 若 key 为 NULL，则使用全零秘钥
 * @return 64-bit hash 值
 */
LMMP_API uint64_t lmmp_xxhash_(mp_srcptr in, mp_size_t inlen, srckey64_t key);

#ifdef __cplusplus
}
#endif

#endif // LMMP_SECRET_H


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

#ifndef __LMMP_IS_PRIME_H__
#define __LMMP_IS_PRIME_H__

#include "../numth.h"

/*
    from http://probableprime.org/download/example-primality.c
    Deterministic Miller-Rabin tests for 64-bit.
    Hashed 2-bases for n < 684630005672341 (slightly more than 2^49)
    Hashed 3-bases for n < 2^64

    Based on Steve Worley's 2^32 example:
       http://www.mersenneforum.org/showthread.php?t=12209
    With a 3-base encoding idea from Bradley Berg.

    Copyright 2014, Dana Jacobsen <dana@acm.org>

    表定义在 is_prime_table.c（单副本，extern 避免各编译单元重复实体）。
*/

/* 4kb hash table：dj_base49[h] + 3 为第二基底，h = hash(n)，确定性覆盖 n < 2^49.3 */
extern const uint16_t dj_base49[2048];

/* 32kb hash table：dj_base64[h] 编码 2/3 基底（bit15 选大基底，低 15 位 + 3 为第三基底） */
extern const uint16_t dj_base64[16384];

/**
 * @brief u32 档双基底强伪素数内核（基底 2 + dj_base49 哈希基底，普通域免蒙域算术）
 * @param n 待判断的数
 * @warning n>2, n%2==1
 * @note 建议调用前已过 3,5,7,11 试除（内部不试除）；
 *       供 lmmp_is_prime_uint_ 与 lmmp_is_prime_notrial_ 的 n≤2^32-1 档共享；
 *       判定集合与 mont63 双链旧实现精确一致（同基底同强 MR 语义）
 * @return 若 n 为素数，返回 true，否则返回 false
 */
bool lmmp_is_prime_uint_notrial_(uint n);

#endif  // __LMMP_IS_PRIME_H__

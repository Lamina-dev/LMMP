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
 *  License along with this library.  If not, see
 *  <https://www.gnu.org/licenses/>.
 *
 *  SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef LMMP_PERF_PRIME_POOL_H
#define LMMP_PERF_PRIME_POOL_H

#include "support/perf.h"
#include "lmmp/lmmpn.h"
#include "lmmp/numth.h"

/*
    素性检验的输入池：单次判定成本对输入性质极度敏感（小因子合数
    试除即拒、素数全基底执行），故以池轮转代替单输入，保证测得
    目标总体的齐次负载：
      ODD     随机奇数（合数为主，平均负载）
      PRIME   素数（全轮完整执行的最坏情形）
      COPRIME 试除幸存者（无 <=1000 素因子；强 Lucas 对含小因子输入
              经 Jacobi gcd 提前退出，且 measure 取最小值会被任一
              快速元素掩盖，须过滤到试除上界之上保证池内齐次）
*/

typedef enum { PRIME_POOL_ODD, PRIME_POOL_PRIME, PRIME_POOL_COPRIME } PrimePool;

enum { POOL = 512 };

static inline int pool_cnt(mp_size_t n) { return n <= 64 ? 32 : 8; }

/**
 * @brief COPRIME 输入池的预筛
 * @note 与 <=1000 素数之积取 gcd，gcd!=1 等价于存在 <=1000 素因子（池元素
 *       恒奇，素数 2 无关）
 */
static inline bool small_factor_q(mp_srcptr x, mp_size_t n) {
    static mp_ptr pr = NULL;
    static mp_size_t prn = 0;
    if (pr == NULL) {
        mp_size_t need = lmmp_primefac_size_(1000);
        pr = alloc_limbs(need);
        prn = lmmp_primefac_(pr, need, 1000);
    }
    mp_ptr g = alloc_limbs(prn);
    mp_size_t gn = (n >= prn) ? lmmp_gcd_(g, x, n, pr, prn) : lmmp_gcd_(g, pr, prn, x, n);
    bool has = !(gn == 1 && g[0] == 1);
    lmmp_free(g);
    return has;
}

/* n limbs 归一化奇数池（元素个数 cnt） */
static inline mp_ptr pool_n(mp_size_t n, int cnt, mp_limb_t seed, PrimePool kind) {
    mp_ptr a = alloc_limbs((size_t)n * cnt);
    for (int i = 0; i < cnt; i++) {
        mp_ptr cur = a + n * i;
        lmmp_seed_random_(cur, n, seed + i * 131, 1);
        cur[n - 1] |= (mp_limb_t)1 << 63;
        cur[0] |= 1;
        if (kind == PRIME_POOL_COPRIME) {
            while (small_factor_q(cur, n)) cur[0] += 2;
        } else if (kind == PRIME_POOL_PRIME) {
            while (lmmp_is_prime_n_(cur, n, 4) == 0) cur[0] += 2;
        }
    }
    return a;
}

#endif

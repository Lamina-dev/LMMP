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

#include "../../../include/lmmp/impl/mul_hard.h"
#include "../../../include/lmmp/impl/mparam.h"
#include "../../../include/lmmp/impl/toom_interp.h"


#ifdef LMMP_TUNE
#define MAYBE_sqr_basecase 1
#define MAYBE_sqr_above_basecase 1
#define MAYBE_sqr_toom2 1
#define MAYBE_sqr_above_toom2 1
#define MAYBE_sqr_toom3 1
#define MAYBE_sqr_above_toom3 1
#define MAYBE_sqr_toom4 1
#define MAYBE_sqr_above_toom4 1
#define MAYBE_sqr_above_toom6 1
#else
/* sqr_toom8 递归可达的最大子问题规模（FFT 接管前） */
#define SQR_TOOM8_MAX ((SQR_FFT_THRESHOLD + 8 * 2 - 1 + 7) / 8)
#define MAYBE_sqr_basecase (SQR_TOOM8_THRESHOLD < 8 * SQR_TOOM22_THRESHOLD)
#define MAYBE_sqr_above_basecase (SQR_TOOM8_MAX >= SQR_TOOM22_THRESHOLD)
#define MAYBE_sqr_toom2 (SQR_TOOM8_THRESHOLD < 8 * SQR_TOOM33_THRESHOLD)
#define MAYBE_sqr_above_toom2 (SQR_TOOM8_MAX >= SQR_TOOM33_THRESHOLD)
#define MAYBE_sqr_toom3 (SQR_TOOM8_THRESHOLD < 8 * SQR_TOOM44_THRESHOLD)
#define MAYBE_sqr_above_toom3 (SQR_TOOM8_MAX >= SQR_TOOM44_THRESHOLD)
#define MAYBE_sqr_toom4 (SQR_TOOM8_THRESHOLD < 8 * SQR_TOOM6_THRESHOLD)
#define MAYBE_sqr_above_toom4 (SQR_TOOM8_MAX >= SQR_TOOM6_THRESHOLD)
#define MAYBE_sqr_above_toom6 (SQR_TOOM8_MAX >= SQR_TOOM8_THRESHOLD)
#endif

#define lmmp_sqr_(dst, numa, n)                                                          \
    if (MAYBE_sqr_basecase && (!MAYBE_sqr_above_basecase || (n) < SQR_TOOM22_THRESHOLD)) \
        lmmp_sqr_hard_n_((dst), (numa), (n));                                            \
    else if (MAYBE_sqr_toom2 && (!MAYBE_sqr_above_toom2 || (n) < SQR_TOOM33_THRESHOLD))  \
        lmmp_sqr_toom2_((dst), (numa), (n));                                             \
    else if (MAYBE_sqr_toom3 && (!MAYBE_sqr_above_toom3 || (n) < SQR_TOOM44_THRESHOLD))  \
        lmmp_sqr_toom3_((dst), (numa), (n));                                             \
    else if (MAYBE_sqr_toom4 && (!MAYBE_sqr_above_toom4 || (n) < SQR_TOOM6_THRESHOLD))   \
        lmmp_sqr_toom4_((dst), (numa), (n));                                             \
    else if (!MAYBE_sqr_above_toom6 || (n) < SQR_TOOM8_THRESHOLD)                        \
        lmmp_sqr_toom6_((dst), (numa), (n));                                             \
    else                                                                                 \
        lmmp_sqr_toom8_((dst), (numa), (n))

/*
Evaluate in: +8,-8, +4,-4, +2,-2, +1,-1, +1/2,-1/2, +1/4,-1/4, +1/8,-1/8, 0
（平方无 inf 点：(-v)^2 = v^2，符号恒正）
*/

void lmmp_sqr_toom8_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na > 0);
    lmmp_param_assert(dst != NULL);
    lmmp_param_assert(numa != NULL);
    lmmp_param_assert(na >= 51); /* 契约排除 s=1 退化点 na=50/57（16 点插值需 2s>3） */
    lmmp_param_assert(na != 57);
    mp_size_t n, s;

    n = 1 + ((na - 1) >> 3);
    s = na - 7 * n;

    lmmp_debug_assert(0 < s && s <= n);
    /* 契约排除 na=50、57（na>=50 中仅此两点 s=1，不满足 2s>3） */
    lmmp_debug_assert(s + s > 3);

#define r6 (dst + 3 * n)           /* 3n+1 */
#define r4 (dst + 7 * n)           /* 3n+1 */
#define r2 (dst + 11 * n)          /* 3n+1 */
#define r0 (dst + 15 * n)          /* 2s */
#define r7 (scratch)               /* 3n+1 */
#define r5 (scratch + 3 * n + 1)   /* 3n+1 */
#define r3 (scratch + 6 * n + 2)   /* 3n+1 */
#define r1 (scratch + 9 * n + 3)   /* 3n+1 */
#define v0 (dst + 11 * n)          /* n+1 */
#define v2 (dst + 13 * n + 2)      /* n+1 */
#define wse (scratch + 12 * n + 4) /* 3n+1 */

    /* scratch 总需求 15n+6 limbs；TALLOC 双轨（>32KiB 落池/堆） */
    TEMP_DECL;
    mp_ptr restrict scratch = TALLOC_TYPE(15 * n + 6, mp_limb_t);

    /* ±1/8 */
    lmmp_toom_eval_pm2rexp_(v2, v0, 7, numa, n, s, 3, dst);
    lmmp_sqr_(dst, v0, n + 1);
    lmmp_sqr_(r7, v2, n + 1);
    lmmp_toom_couple_handling_(r7, 2 * n + 1, dst, 0, n, 3, 0);

    /* ±1/4 */
    lmmp_toom_eval_pm2rexp_(v2, v0, 7, numa, n, s, 2, dst);
    lmmp_sqr_(dst, v0, n + 1);
    lmmp_sqr_(r5, v2, n + 1);
    lmmp_toom_couple_handling_(r5, 2 * n + 1, dst, 0, n, 2, 0);

    /* ±2 */
    lmmp_toom_eval_pm2_(v2, v0, 7, numa, n, s, dst);
    lmmp_sqr_(dst, v0, n + 1);
    lmmp_sqr_(r3, v2, n + 1);
    lmmp_toom_couple_handling_(r3, 2 * n + 1, dst, 0, n, 1, 2);

    /* ±8 */
    lmmp_toom_eval_pm2exp_(v2, v0, 7, numa, n, s, 3, dst);
    lmmp_sqr_(dst, v0, n + 1);
    lmmp_sqr_(r1, v2, n + 1);
    lmmp_toom_couple_handling_(r1, 2 * n + 1, dst, 0, n, 3, 6);

    /* ±1/2 */
    lmmp_toom_eval_pm2rexp_(v2, v0, 7, numa, n, s, 1, dst);
    lmmp_sqr_(dst, v0, n + 1);
    lmmp_sqr_(r6, v2, n + 1);
    lmmp_toom_couple_handling_(r6, 2 * n + 1, dst, 0, n, 1, 0);

    /* ±1 */
    lmmp_toom_eval_pm1_(v2, v0, 7, numa, n, s, dst);
    lmmp_sqr_(dst, v0, n + 1);
    lmmp_sqr_(r4, v2, n + 1);
    lmmp_toom_couple_handling_(r4, 2 * n + 1, dst, 0, n, 0, 0);

    /* ±4 */
    lmmp_toom_eval_pm2exp_(v2, v0, 7, numa, n, s, 2, dst);
    lmmp_sqr_(dst, v0, n + 1);
    lmmp_sqr_(r2, v2, n + 1);
    lmmp_toom_couple_handling_(r2, 2 * n + 1, dst, 0, n, 2, 4);

#undef v0
#undef v2

    /* A(0)^2 */
    lmmp_sqr_(dst, numa, n);

    lmmp_toom_interp16_(dst, r1, r3, r5, r7, n, 2 * s, 0, wse);

#undef r0
#undef r1
#undef r2
#undef r3
#undef r4
#undef r5
#undef r6
#undef r7

    TEMP_FREE;
}

#undef lmmp_sqr_
#undef MAYBE_sqr_basecase
#undef MAYBE_sqr_above_basecase
#undef MAYBE_sqr_toom2
#undef MAYBE_sqr_above_toom2
#undef MAYBE_sqr_toom3
#undef MAYBE_sqr_above_toom3
#undef MAYBE_sqr_toom4
#undef MAYBE_sqr_above_toom4
#undef MAYBE_sqr_above_toom6
#ifndef LMMP_TUNE
#undef SQR_TOOM8_MAX
#endif

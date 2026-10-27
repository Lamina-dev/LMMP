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
#define MAYBE_mul_basecase 1
#define MAYBE_mul_toom22 1
#define MAYBE_mul_toom33 1
#define MAYBE_mul_toom44 1
#define MAYBE_mul_toom8h 1
#else
#define MAYBE_mul_basecase (MUL_TOOM8H_THRESHOLD < 8 * MUL_TOOM22_THRESHOLD)
#define MAYBE_mul_toom22 (MUL_TOOM8H_THRESHOLD < 8 * MUL_TOOM33_THRESHOLD)
#define MAYBE_mul_toom33 (MUL_TOOM8H_THRESHOLD < 8 * MUL_TOOM44_THRESHOLD)
#define MAYBE_mul_toom44 (MUL_TOOM8H_THRESHOLD < 8 * MUL_TOOM6H_THRESHOLD)
#define MAYBE_mul_toom8h (MUL_FFT_THRESHOLD >= 8 * MUL_TOOM8H_THRESHOLD)
#endif

#define lmmp_mul_n_(dst, numa, numb, n)                       \
    if (MAYBE_mul_basecase && (n) < MUL_TOOM22_THRESHOLD)     \
        lmmp_mul_hard_n_((dst), (numa), (numb), (n));         \
    else if (MAYBE_mul_toom22 && (n) < MUL_TOOM33_THRESHOLD)  \
        lmmp_mul_toom22_((dst), (numa), (n), (numb), (n));    \
    else if (MAYBE_mul_toom33 && (n) < MUL_TOOM44_THRESHOLD)  \
        lmmp_mul_toom33_((dst), (numa), (n), (numb), (n));    \
    else if (MAYBE_mul_toom44 && (n) < MUL_TOOM6H_THRESHOLD)  \
        lmmp_mul_toom44_((dst), (numa), (n), (numb), (n));    \
    else if (!MAYBE_mul_toom8h || (n) < MUL_TOOM8H_THRESHOLD) \
        lmmp_mul_toom6h_((dst), (numa), (n), (numb), (n));    \
    else                                                      \
        lmmp_mul_toom8h_((dst), (numa), (n), (numb), (n))

/*
Evaluate in: infinity, +8,-8, +4,-4, +2,-2, +1,-1, +1/2,-1/2, +1/4,-1/4, +1/8,-1/8, 0
*/

void lmmp_mul_toom8h_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(nb >= 86);
    lmmp_param_assert(na <= 4 * nb);
    mp_size_t n, s, t;
    int p, q, half;
    int sign;

    /* LIMIT = 21/20：介于 (16/15)^(log6/log11) 与 (16/15)^(log8/log15)
       之间的有理逼近（比例 < 21/20 视为近似平衡） */
#define LIMIT_num (21)
#define LIMIT_den (20)

    if (na == nb || na * (LIMIT_den / 2) < LIMIT_num * (nb / 2)) {
        /* 近似平衡：双侧各切 7 段 */
        half = 0;
        n = 1 + ((na - 1) >> 3);
        p = q = 7;
        s = na - 7 * n;
        t = nb - 7 * n;
    } else {
        /* 不平衡阶梯：(p,q) 覆盖比例至 4 */
        if (na * 13 < 16 * nb) {
            p = 9;
            q = 8;
        } else if (na * (LIMIT_den / 2) < (LIMIT_num / 7 * 9) * (nb / 2)) {
            p = 9;
            q = 7;
        } else if (na * 10 < 33 * (nb / 2)) {
            p = 10;
            q = 7;
        } else if (na * (LIMIT_den / 5) < (LIMIT_num / 3) * nb) {
            p = 10;
            q = 6;
        } else if (na * 6 < 13 * nb) {
            p = 11;
            q = 6;
        } else if (na * 4 < 9 * nb) {
            p = 11;
            q = 5;
        } else if (na * (LIMIT_num / 3) < LIMIT_den * nb) {
            p = 12;
            q = 5;
        } else if (na * 9 < 28 * nb) {
            p = 12;
            q = 4;
        } else {
            p = 13;
            q = 4;
        }

        half = (p + q) & 1;
        n = 1 + (q * na >= p * nb ? (na - 1) / p : (nb - 1) / q);
        p--;
        q--;

        s = na - p * n;
        t = nb - q * n;

        /* 恢复不良切分。mp_size_t 无符号，s/t 为负时回绕，
           须按 na<=p*n / nb<=q*n 判定 */
        if (half) {
            if (na <= (mp_size_t)p * n) {
                p--;
                s += n;
                half = 0;
            } else if (nb <= (mp_size_t)q * n) {
                q--;
                t += n;
                half = 0;
            }
        }
    }
#undef LIMIT_num
#undef LIMIT_den

    lmmp_debug_assert(0 < s && s <= n);
    lmmp_debug_assert(0 < t && t <= n);
    lmmp_debug_assert(s + t + 3 * (mp_size_t)half > 3);
    lmmp_debug_assert(n > 2);

#define r6 (dst + 3 * n)           /* 3n+1 */
#define r4 (dst + 7 * n)           /* 3n+1 */
#define r2 (dst + 11 * n)          /* 3n+1 */
#define r0 (dst + 15 * n)          /* s+t <= 2n */
#define r7 (scratch)               /* 3n+1 */
#define r5 (scratch + 3 * n + 1)   /* 3n+1 */
#define r3 (scratch + 6 * n + 2)   /* 3n+1 */
#define r1 (scratch + 9 * n + 3)   /* 3n+1 */
#define v0 (dst + 11 * n)          /* n+1 */
#define v1 (dst + 12 * n + 1)      /* n+1 */
#define v2 (dst + 13 * n + 2)      /* n+1 */
#define v3 (scratch + 12 * n + 4)  /* n+1 */
#define wsi (scratch + 12 * n + 4) /* 3n+1，与 v3 复用（生命周期不交） */
#define wse (scratch + 13 * n + 5) /* 2n+1 */

    /* scratch 总需求 15n+6 limbs；TALLOC 双轨（>32KiB 落池/堆） */
    TEMP_DECL;
    mp_ptr restrict scratch = TALLOC_TYPE(15 * n + 6, mp_limb_t);

    /* ±1/8 */
    sign = lmmp_toom_eval_pm2rexp_(v2, v0, p, numa, n, s, 3, dst) ^ lmmp_toom_eval_pm2rexp_(v3, v1, q, numb, n, t, 3, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r7, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r7, 2 * n + 1, dst, sign, n, 3 * (1 + half), 3 * half);

    /* ±1/4 */
    sign = lmmp_toom_eval_pm2rexp_(v2, v0, p, numa, n, s, 2, dst) ^ lmmp_toom_eval_pm2rexp_(v3, v1, q, numb, n, t, 2, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r5, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r5, 2 * n + 1, dst, sign, n, 2 * (1 + half), 2 * half);

    /* ±2 */
    sign = lmmp_toom_eval_pm2_(v2, v0, p, numa, n, s, dst) ^ lmmp_toom_eval_pm2_(v3, v1, q, numb, n, t, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r3, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r3, 2 * n + 1, dst, sign, n, 1, 2);

    /* ±8 */
    sign = lmmp_toom_eval_pm2exp_(v2, v0, p, numa, n, s, 3, dst) ^ lmmp_toom_eval_pm2exp_(v3, v1, q, numb, n, t, 3, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r1, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r1, 2 * n + 1, dst, sign, n, 3, 6);

    /* ±1/2 */
    sign = lmmp_toom_eval_pm2rexp_(v2, v0, p, numa, n, s, 1, dst) ^ lmmp_toom_eval_pm2rexp_(v3, v1, q, numb, n, t, 1, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r6, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r6, 2 * n + 1, dst, sign, n, 1 + half, half);

    /* ±1 */
    sign = lmmp_toom_eval_pm1_(v2, v0, p, numa, n, s, dst);
    if (q == 3)
        sign ^= lmmp_toom_eval_dgr3_pm1_(v3, v1, numb, n, t, dst);
    else
        sign ^= lmmp_toom_eval_pm1_(v3, v1, q, numb, n, t, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r4, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r4, 2 * n + 1, dst, sign, n, 0, 0);

    /* ±4 */
    sign = lmmp_toom_eval_pm2exp_(v2, v0, p, numa, n, s, 2, dst) ^ lmmp_toom_eval_pm2exp_(v3, v1, q, numb, n, t, 2, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r2, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r2, 2 * n + 1, dst, sign, n, 2, 4);

#undef v0
#undef v1
#undef v2
#undef v3
#undef wse

    /* A(0)*B(0) */
    lmmp_mul_n_(dst, numa, numb, n);

    /* 无穷远点 */
    if (half != 0) {
        if (s > t)
            lmmp_mul_(r0, numa + p * n, s, numb + q * n, t);
        else
            lmmp_mul_(r0, numb + q * n, t, numa + p * n, s);
    }

    lmmp_toom_interp16_(dst, r1, r3, r5, r7, n, s + t, half, wsi);

#undef r0
#undef r1
#undef r2
#undef r3
#undef r4
#undef r5
#undef r6
#undef r7
#undef wsi

    TEMP_FREE;
}

#undef lmmp_mul_n_
#undef MAYBE_mul_basecase
#undef MAYBE_mul_toom22
#undef MAYBE_mul_toom33
#undef MAYBE_mul_toom44
#undef MAYBE_mul_toom8h

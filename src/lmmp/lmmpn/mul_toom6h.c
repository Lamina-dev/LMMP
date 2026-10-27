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


/* MAYBE_*：编译期常量折叠裁剪递归阶梯（调优构建阈值是运行时变量，全开） */
#ifdef LMMP_TUNE
#define MAYBE_mul_basecase 1
#define MAYBE_mul_toom22 1
#define MAYBE_mul_toom33 1
#define MAYBE_mul_toom6h 1
#else
#define MAYBE_mul_basecase (MUL_TOOM6H_THRESHOLD < 6 * MUL_TOOM22_THRESHOLD)
#define MAYBE_mul_toom22 (MUL_TOOM6H_THRESHOLD < 6 * MUL_TOOM33_THRESHOLD)
#define MAYBE_mul_toom33 (MUL_TOOM6H_THRESHOLD < 6 * MUL_TOOM44_THRESHOLD)
#define MAYBE_mul_toom6h (MUL_FFT_THRESHOLD >= 6 * MUL_TOOM6H_THRESHOLD)
#endif

#define lmmp_mul_n_(dst, numa, numb, n)                       \
    if (MAYBE_mul_basecase && (n) < MUL_TOOM22_THRESHOLD)     \
        lmmp_mul_hard_n_((dst), (numa), (numb), (n));         \
    else if (MAYBE_mul_toom22 && (n) < MUL_TOOM33_THRESHOLD)  \
        lmmp_mul_toom22_((dst), (numa), (n), (numb), (n));    \
    else if (MAYBE_mul_toom33 && (n) < MUL_TOOM44_THRESHOLD)  \
        lmmp_mul_toom33_((dst), (numa), (n), (numb), (n));    \
    else if (!MAYBE_mul_toom6h || (n) < MUL_TOOM6H_THRESHOLD) \
        lmmp_mul_toom44_((dst), (numa), (n), (numb), (n));    \
    else                                                      \
        lmmp_mul_toom6h_((dst), (numa), (n), (numb), (n))

/*
Evaluate in: infinity, +4, -4, +2, -2, +1, -1, +1/2, -1/2, +1/4, -1/4, 0

  <-s-><---n---><---n---><---n---><--n-1--> ... (p 段)
  |a_p|---a_p-1-|---...---|---a1---|---a0---|
   |b_q|--...----|---b1---|---b0---|         (q 段, q <= p)

  v(0)   = a0*b0
  v(±1)  = A(±1)*B(±1)          |A(±1)|  <= p
  v(±2)  = A(±2)*B(±2)          2^k 求值约束: 2*k_max < LIMB_BITS
  v(±4)  = A(±4)*B(±4)
  v(±1/2)= A(±1/2)*B(±1/2)*2^k
  v(±1/4)= A(±1/4)*B(±1/4)*4^k
  v(inf) = a_top*b_top（仅 half，走 lmmp_mul_ 完整分派）
*/

void lmmp_mul_toom6h_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na, mp_srcptr restrict numb, mp_size_t nb) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(nb >= 46);
    lmmp_param_assert(6 * na < 17 * nb);
    mp_size_t n, s, t;
    int p, q, half;
    int sign;

    /* LIMIT = 18/17：介于 (12/11)^(log4/log7) 与 (12/11)^(log6/log11)
       之间的有理逼近，控制近似平衡切分的判定边界（比例 < 18/17） */
#define LIMIT_num (18)
#define LIMIT_den (17)

    if (na * LIMIT_den < LIMIT_num * nb) {
        /* 近似平衡：双侧各切 5 段 */
        n = 1 + (na - 1) / 6;
        p = q = 5;
        half = 0;
        s = na - 5 * n;
        t = nb - 5 * n;
    } else {
        /* 不平衡阶梯：(p,q) 随比例增大，短侧段数下降 */
        if (na * 5 * LIMIT_num < LIMIT_den * 7 * nb) {
            p = 7;
            q = 6;
        } else if (na * 5 * LIMIT_den < LIMIT_num * 7 * nb) {
            p = 7;
            q = 5;
        } else if (na * LIMIT_num < LIMIT_den * 2 * nb) {
            p = 8;
            q = 5;
        } else if (na * LIMIT_den < LIMIT_num * 2 * nb) {
            p = 8;
            q = 4;
        } else {
            p = 9;
            q = 4;
        }

        half = (p ^ q) & 1;
        n = 1 + (q * na >= p * nb ? (na - 1) / p : (nb - 1) / q);
        p--;
        q--;

        s = na - p * n;
        t = nb - q * n;

        /* 恢复不良切分：段长必须保证 s,t > 0（LIMIT=18/17 时仅 nb<=73 触发）。
           mp_size_t 无符号，s/t 为负时回绕，须按 na<=p*n / nb<=q*n 判定 */
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

#define r4 (dst + 3 * n)          /* 3n+1 */
#define r2 (dst + 7 * n)          /* 3n+1 */
#define r0 (dst + 11 * n)         /* s+t <= 2n */
#define r5 (scratch)              /* 3n+1 */
#define r3 (scratch + 3 * n + 1)  /* 3n+1 */
#define r1 (scratch + 6 * n + 2)  /* 3n+1 */
#define v0 (dst + 7 * n)          /* n+1 */
#define v1 (dst + 8 * n + 1)      /* n+1 */
#define v2 (dst + 9 * n + 2)      /* n+1 */
#define v3 (scratch + 9 * n + 3)  /* n+1 */
#define wsi (scratch + 9 * n + 3) /* 3n+1，与 v3 复用（生命周期不交） */
#define wse (scratch + 10 * n + 4) /* 2n+1 */

    /* scratch 总需求 12n+6 limbs；超过 32KiB 自动落池/堆（TALLOC 双轨），
       最坏递归栈按比例收敛（子层 n' <= (n+5)/6，toom44 子层 SALLOC 10n'） */
    TEMP_DECL;
    mp_ptr restrict scratch = TALLOC_TYPE(12 * n + 6, mp_limb_t);

    /* ±1/2 */
    sign = lmmp_toom_eval_pm2rexp_(v2, v0, p, numa, n, s, 1, dst) ^ lmmp_toom_eval_pm2rexp_(v3, v1, q, numb, n, t, 1, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r5, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r5, 2 * n + 1, dst, sign, n, 1 + half, half);

    /* ±1 */
    sign = lmmp_toom_eval_pm1_(v2, v0, p, numa, n, s, dst);
    if (q == 3)
        sign ^= lmmp_toom_eval_dgr3_pm1_(v3, v1, numb, n, t, dst);
    else
        sign ^= lmmp_toom_eval_pm1_(v3, v1, q, numb, n, t, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r3, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r3, 2 * n + 1, dst, sign, n, 0, 0);

    /* ±4 */
    sign = lmmp_toom_eval_pm2exp_(v2, v0, p, numa, n, s, 2, dst) ^ lmmp_toom_eval_pm2exp_(v3, v1, q, numb, n, t, 2, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r1, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r1, 2 * n + 1, dst, sign, n, 2, 4);

    /* ±1/4 */
    sign = lmmp_toom_eval_pm2rexp_(v2, v0, p, numa, n, s, 2, dst) ^ lmmp_toom_eval_pm2rexp_(v3, v1, q, numb, n, t, 2, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r4, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r4, 2 * n + 1, dst, sign, n, 2 * (1 + half), 2 * half);

    /* ±2 */
    sign = lmmp_toom_eval_pm2_(v2, v0, p, numa, n, s, dst) ^ lmmp_toom_eval_pm2_(v3, v1, q, numb, n, t, dst);
    lmmp_mul_n_(dst, v0, v1, n + 1);
    lmmp_mul_n_(r2, v2, v3, n + 1);
    lmmp_toom_couple_handling_(r2, 2 * n + 1, dst, sign, n, 1, 2);

#undef v0
#undef v1
#undef v2
#undef v3
#undef wse

    /* A(0)*B(0) */
    lmmp_mul_n_(dst, numa, numb, n);

    /* 无穷远点：顶部 s*t 尾巴之积，较大者在前 */
    if (half != 0) {
        if (s > t)
            lmmp_mul_(r0, numa + p * n, s, numb + q * n, t);
        else
            lmmp_mul_(r0, numb + q * n, t, numa + p * n, s);
    }

    lmmp_toom_interp12_(dst, r1, r3, r5, n, s + t, half, wsi);

#undef r0
#undef r1
#undef r2
#undef r3
#undef r4
#undef r5
#undef wsi

    TEMP_FREE;
}

#undef lmmp_mul_n_
#undef MAYBE_mul_basecase
#undef MAYBE_mul_toom22
#undef MAYBE_mul_toom33
#undef MAYBE_mul_toom6h

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

#include "../../../../include/lmmp/impl/longlong.h"
#include "../../../../include/lmmp/lmmpn.h"

/*
    __SIZEOF_INT128__ 路径：列内乘加用 __uint128_t 单条表达式书写，
    编译器生成 mul + add/adc 短链，乘法可乱序提前发射；首列为纯
    mul_1 型（无 dst 累加），其余列为 addmul_1 型。两列合并外层
    循环减少指针推进与循环开销。
*/

#if defined(__SIZEOF_INT128__)

void lmmp_sqr_basecase_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na >= 1);

    mp_size_t i;
    mp_limb_t cl, x;

    /* 首列：dst[0..na] = numa[0] * numa */
    x = numa[0];
    cl = 0;
    for (i = 0; i + 4 <= na; i += 4) {
        __uint128_t a0 = (__uint128_t)numa[i + 0] * x + cl;
        __uint128_t a1 = (__uint128_t)numa[i + 1] * x + (uint64_t)(a0 >> 64);
        __uint128_t a2 = (__uint128_t)numa[i + 2] * x + (uint64_t)(a1 >> 64);
        __uint128_t a3 = (__uint128_t)numa[i + 3] * x + (uint64_t)(a2 >> 64);
        dst[i + 0] = (uint64_t)a0;
        dst[i + 1] = (uint64_t)a1;
        dst[i + 2] = (uint64_t)a2;
        dst[i + 3] = (uint64_t)a3;
        cl = (uint64_t)(a3 >> 64);
    }
    for (; i < na; i++) {
        __uint128_t a = (__uint128_t)numa[i] * x + cl;
        dst[i] = (uint64_t)a;
        cl = (uint64_t)(a >> 64);
    }
    dst[na] = cl;

    /* 其余列：dst[1..na-1] 对应乘数 numa[1..na-1]，逐列 addmul 累加 */
    mp_size_t j = 1;
    mp_ptr dp = dst + 1;
    for (; j + 2 <= na; j += 2, dp += 2) {
        cl = 0;
        x = numa[j];
        for (i = 0; i + 4 <= na; i += 4) {
            __uint128_t a0 = (__uint128_t)numa[i + 0] * x + dp[i + 0] + cl;
            __uint128_t a1 = (__uint128_t)numa[i + 1] * x + dp[i + 1] + (uint64_t)(a0 >> 64);
            __uint128_t a2 = (__uint128_t)numa[i + 2] * x + dp[i + 2] + (uint64_t)(a1 >> 64);
            __uint128_t a3 = (__uint128_t)numa[i + 3] * x + dp[i + 3] + (uint64_t)(a2 >> 64);
            dp[i + 0] = (uint64_t)a0;
            dp[i + 1] = (uint64_t)a1;
            dp[i + 2] = (uint64_t)a2;
            dp[i + 3] = (uint64_t)a3;
            cl = (uint64_t)(a3 >> 64);
        }
        for (; i < na; i++) {
            __uint128_t a = (__uint128_t)numa[i] * x + dp[i] + cl;
            dp[i] = (uint64_t)a;
            cl = (uint64_t)(a >> 64);
        }
        dp[na] = cl;

        cl = 0;
        x = numa[j + 1];
        for (i = 0; i + 4 <= na; i += 4) {
            __uint128_t a0 = (__uint128_t)numa[i + 0] * x + dp[i + 1] + cl;
            __uint128_t a1 = (__uint128_t)numa[i + 1] * x + dp[i + 2] + (uint64_t)(a0 >> 64);
            __uint128_t a2 = (__uint128_t)numa[i + 2] * x + dp[i + 3] + (uint64_t)(a1 >> 64);
            __uint128_t a3 = (__uint128_t)numa[i + 3] * x + dp[i + 4] + (uint64_t)(a2 >> 64);
            dp[i + 1] = (uint64_t)a0;
            dp[i + 2] = (uint64_t)a1;
            dp[i + 3] = (uint64_t)a2;
            dp[i + 4] = (uint64_t)a3;
            cl = (uint64_t)(a3 >> 64);
        }
        for (; i < na; i++) {
            __uint128_t a = (__uint128_t)numa[i] * x + dp[i + 1] + cl;
            dp[i + 1] = (uint64_t)a;
            cl = (uint64_t)(a >> 64);
        }
        dp[na + 1] = cl;
    }
    for (; j < na; j++, dp++) {
        cl = 0;
        x = numa[j];
        for (i = 0; i + 4 <= na; i += 4) {
            __uint128_t a0 = (__uint128_t)numa[i + 0] * x + dp[i + 0] + cl;
            __uint128_t a1 = (__uint128_t)numa[i + 1] * x + dp[i + 1] + (uint64_t)(a0 >> 64);
            __uint128_t a2 = (__uint128_t)numa[i + 2] * x + dp[i + 2] + (uint64_t)(a1 >> 64);
            __uint128_t a3 = (__uint128_t)numa[i + 3] * x + dp[i + 3] + (uint64_t)(a2 >> 64);
            dp[i + 0] = (uint64_t)a0;
            dp[i + 1] = (uint64_t)a1;
            dp[i + 2] = (uint64_t)a2;
            dp[i + 3] = (uint64_t)a3;
            cl = (uint64_t)(a3 >> 64);
        }
        for (; i < na; i++) {
            __uint128_t a = (__uint128_t)numa[i] * x + dp[i] + cl;
            dp[i] = (uint64_t)a;
            cl = (uint64_t)(a >> 64);
        }
        dp[na] = cl;
    }
}

void lmmp_mul_basecase_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict  numa,
    mp_size_t            na,
    mp_srcptr restrict  numb,
    mp_size_t            nb
) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(nb >= 1);

    mp_size_t i;
    mp_limb_t cl, x;

    /* 首列：dst[0..na] = numb[0] * numa */
    x = numb[0];
    cl = 0;
    for (i = 0; i + 4 <= na; i += 4) {
        __uint128_t a0 = (__uint128_t)numa[i + 0] * x + cl;
        __uint128_t a1 = (__uint128_t)numa[i + 1] * x + (uint64_t)(a0 >> 64);
        __uint128_t a2 = (__uint128_t)numa[i + 2] * x + (uint64_t)(a1 >> 64);
        __uint128_t a3 = (__uint128_t)numa[i + 3] * x + (uint64_t)(a2 >> 64);
        dst[i + 0] = (uint64_t)a0;
        dst[i + 1] = (uint64_t)a1;
        dst[i + 2] = (uint64_t)a2;
        dst[i + 3] = (uint64_t)a3;
        cl = (uint64_t)(a3 >> 64);
    }
    for (; i < na; i++) {
        __uint128_t a = (__uint128_t)numa[i] * x + cl;
        dst[i] = (uint64_t)a;
        cl = (uint64_t)(a >> 64);
    }
    dst[na] = cl;

    /* 其余列：逐列 addmul 累加，两列合并外层循环 */
    mp_size_t j = 1;
    mp_ptr dp = dst + 1;
    nb -= 1;
    for (; nb >= 2; j += 2, dp += 2, nb -= 2) {
        cl = 0;
        x = numb[j];
        for (i = 0; i + 4 <= na; i += 4) {
            __uint128_t a0 = (__uint128_t)numa[i + 0] * x + dp[i + 0] + cl;
            __uint128_t a1 = (__uint128_t)numa[i + 1] * x + dp[i + 1] + (uint64_t)(a0 >> 64);
            __uint128_t a2 = (__uint128_t)numa[i + 2] * x + dp[i + 2] + (uint64_t)(a1 >> 64);
            __uint128_t a3 = (__uint128_t)numa[i + 3] * x + dp[i + 3] + (uint64_t)(a2 >> 64);
            dp[i + 0] = (uint64_t)a0;
            dp[i + 1] = (uint64_t)a1;
            dp[i + 2] = (uint64_t)a2;
            dp[i + 3] = (uint64_t)a3;
            cl = (uint64_t)(a3 >> 64);
        }
        for (; i < na; i++) {
            __uint128_t a = (__uint128_t)numa[i] * x + dp[i] + cl;
            dp[i] = (uint64_t)a;
            cl = (uint64_t)(a >> 64);
        }
        dp[na] = cl;

        cl = 0;
        x = numb[j + 1];
        for (i = 0; i + 4 <= na; i += 4) {
            __uint128_t a0 = (__uint128_t)numa[i + 0] * x + dp[i + 1] + cl;
            __uint128_t a1 = (__uint128_t)numa[i + 1] * x + dp[i + 2] + (uint64_t)(a0 >> 64);
            __uint128_t a2 = (__uint128_t)numa[i + 2] * x + dp[i + 3] + (uint64_t)(a1 >> 64);
            __uint128_t a3 = (__uint128_t)numa[i + 3] * x + dp[i + 4] + (uint64_t)(a2 >> 64);
            dp[i + 1] = (uint64_t)a0;
            dp[i + 2] = (uint64_t)a1;
            dp[i + 3] = (uint64_t)a2;
            dp[i + 4] = (uint64_t)a3;
            cl = (uint64_t)(a3 >> 64);
        }
        for (; i < na; i++) {
            __uint128_t a = (__uint128_t)numa[i] * x + dp[i + 1] + cl;
            dp[i + 1] = (uint64_t)a;
            cl = (uint64_t)(a >> 64);
        }
        dp[na + 1] = cl;
    }
    for (; nb >= 1; j++, dp++, nb--) {
        cl = 0;
        x = numb[j];
        for (i = 0; i + 4 <= na; i += 4) {
            __uint128_t a0 = (__uint128_t)numa[i + 0] * x + dp[i + 0] + cl;
            __uint128_t a1 = (__uint128_t)numa[i + 1] * x + dp[i + 1] + (uint64_t)(a0 >> 64);
            __uint128_t a2 = (__uint128_t)numa[i + 2] * x + dp[i + 2] + (uint64_t)(a1 >> 64);
            __uint128_t a3 = (__uint128_t)numa[i + 3] * x + dp[i + 3] + (uint64_t)(a2 >> 64);
            dp[i + 0] = (uint64_t)a0;
            dp[i + 1] = (uint64_t)a1;
            dp[i + 2] = (uint64_t)a2;
            dp[i + 3] = (uint64_t)a3;
            cl = (uint64_t)(a3 >> 64);
        }
        for (; i < na; i++) {
            __uint128_t a = (__uint128_t)numa[i] * x + dp[i] + cl;
            dp[i] = (uint64_t)a;
            cl = (uint64_t)(a >> 64);
        }
        dp[na] = cl;
    }
}

#else /* !__SIZEOF_INT128__ 由 mul_1/addmul_1 组合的朴素参考实现 */

void lmmp_sqr_basecase_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_size_t na) {
    lmmp_param_assert(na >= 1);

    dst[na] = lmmp_mul_1_(dst, numa, na, numa[0]);
    mp_size_t j;
    for (j = 1; j < na; j++) dst[na + j] = lmmp_addmul_1_(dst + j, numa, na, numa[j]);
}

void lmmp_mul_basecase_(
    mp_ptr    restrict  dst,
    mp_srcptr restrict  numa,
    mp_size_t            na,
    mp_srcptr restrict  numb,
    mp_size_t            nb
) {
    lmmp_param_assert(na >= nb);
    lmmp_param_assert(nb >= 1);

    dst[na] = lmmp_mul_1_(dst, numa, na, numb[0]);
    mp_size_t j;
    for (j = 1; j < nb; j++) dst[na + j] = lmmp_addmul_1_(dst + j, numa, na, numb[j]);
}

#endif /* __SIZEOF_INT128__ */

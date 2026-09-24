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


mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x) {
    /*
        链式 (a_{k-1} >> 64) 进位提取使 GCC/Clang 均生成 add/adc 融合序列，
        单 limb 链长约 2c，展开宽度 W 可将链摊薄为 (W+1)/W c：
          - clang 会把 4 路链式源码回卷成 2 路（实测慢约 7~12%），6 路是其
            能保持的最小宽度；
          - gcc 恰好相反，4 路最优、6 路慢约 4%。
        故按编译器选择展开宽度。
    */
    mp_limb_t cl = 0;
    mp_size_t i = 0;

#if defined(__clang__)
    for (; i + 6 <= na; i += 6) {
        __uint128_t a0 = (__uint128_t)numa[i + 0] * x + cl;
        __uint128_t a1 = (__uint128_t)numa[i + 1] * x + (mp_limb_t)(a0 >> 64);
        __uint128_t a2 = (__uint128_t)numa[i + 2] * x + (mp_limb_t)(a1 >> 64);
        __uint128_t a3 = (__uint128_t)numa[i + 3] * x + (mp_limb_t)(a2 >> 64);
        __uint128_t a4 = (__uint128_t)numa[i + 4] * x + (mp_limb_t)(a3 >> 64);
        __uint128_t a5 = (__uint128_t)numa[i + 5] * x + (mp_limb_t)(a4 >> 64);
        dst[i + 0] = (mp_limb_t)a0;
        dst[i + 1] = (mp_limb_t)a1;
        dst[i + 2] = (mp_limb_t)a2;
        dst[i + 3] = (mp_limb_t)a3;
        dst[i + 4] = (mp_limb_t)a4;
        dst[i + 5] = (mp_limb_t)a5;
        cl = (mp_limb_t)(a5 >> 64);
    }
#else
    for (; i + 4 <= na; i += 4) {
        __uint128_t a0 = (__uint128_t)numa[i + 0] * x + cl;
        __uint128_t a1 = (__uint128_t)numa[i + 1] * x + (mp_limb_t)(a0 >> 64);
        __uint128_t a2 = (__uint128_t)numa[i + 2] * x + (mp_limb_t)(a1 >> 64);
        __uint128_t a3 = (__uint128_t)numa[i + 3] * x + (mp_limb_t)(a2 >> 64);
        dst[i + 0] = (mp_limb_t)a0;
        dst[i + 1] = (mp_limb_t)a1;
        dst[i + 2] = (mp_limb_t)a2;
        dst[i + 3] = (mp_limb_t)a3;
        cl = (mp_limb_t)(a3 >> 64);
    }
#endif
    for (; i < na; i++) {
        __uint128_t a = (__uint128_t)numa[i] * x + cl;
        dst[i] = (mp_limb_t)a;
        cl = (mp_limb_t)(a >> 64);
    }
    return cl;
}

mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b) {
    mp_limb_t cl = 0;
    mp_size_t i = 0;

    for (; i + 4 <= n; i += 4) {
        __uint128_t a0 = (__uint128_t)numb[i + 0] * b + numa[i + 0] + cl;
        __uint128_t a1 = (__uint128_t)numb[i + 1] * b + numa[i + 1] + (mp_limb_t)(a0 >> 64);
        __uint128_t a2 = (__uint128_t)numb[i + 2] * b + numa[i + 2] + (mp_limb_t)(a1 >> 64);
        __uint128_t a3 = (__uint128_t)numb[i + 3] * b + numa[i + 3] + (mp_limb_t)(a2 >> 64);
        numa[i + 0] = (mp_limb_t)a0;
        numa[i + 1] = (mp_limb_t)a1;
        numa[i + 2] = (mp_limb_t)a2;
        numa[i + 3] = (mp_limb_t)a3;
        cl = (mp_limb_t)(a3 >> 64);
    }
    for (; i < n; i++) {
        __uint128_t a = (__uint128_t)numb[i] * b + numa[i] + cl;
        numa[i] = (mp_limb_t)a;
        cl = (mp_limb_t)(a >> 64);
    }
    return cl;
}

/*
    双借位分解（与 addmul_1 的 add/adc 结构对称，便于编译器生成短进位链）：

        u = numa[i] - lo(numb[i]*b)          b1 = [u > o]   （与 d 无关，可提前）
        r = u - d                            b2 = [r > u]
        d' = hi(numb[i]*b) + b1 + b2

    正确性：r = numa[i] - lo - d (mod B)；债务 d' = hi + ceil((lo + d - numa)/B)。
    由于 lo + d <= 2B-2，ceil 项可取 0/1/2，恰为 b1 + b2（b1、b2 分别对应两次
    减法各自的回绕，可同时为 1），故 d' <= B-1 仍为完整 limb 借位。
    相比 128 位回绕减写法（sub/sbb/sub/sbb/neg 长串行链），本写法将
    「hi 累积」与「借位判定」拆为两条独立链，GCC/Clang 均能更好调度。
    展开 8 路：clang 下 4/8 路性能相同，gcc 下 4 路实测慢约 25%，故统一取 8 路。
*/
mp_limb_t lmmp_submul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b) {
    mp_limb_t d = 0;
    mp_size_t i = 0;

#define LMMP_SUBMUL_1_STEP(k)                           \
    {                                                   \
        __uint128_t P = (__uint128_t)numb[i + (k)] * b; \
        mp_limb_t o = numa[i + (k)];                    \
        mp_limb_t u = o - (mp_limb_t)P;                 \
        mp_limb_t r = u - d;                            \
        d = (mp_limb_t)(P >> 64) + (u > o) + (r > u);   \
        numa[i + (k)] = r;                              \
    }

    for (; i + 8 <= n; i += 8) {
        LMMP_SUBMUL_1_STEP(0)
        LMMP_SUBMUL_1_STEP(1)
        LMMP_SUBMUL_1_STEP(2)
        LMMP_SUBMUL_1_STEP(3)
        LMMP_SUBMUL_1_STEP(4)
        LMMP_SUBMUL_1_STEP(5)
        LMMP_SUBMUL_1_STEP(6)
        LMMP_SUBMUL_1_STEP(7)
    }
    for (; i < n; i++) {
        LMMP_SUBMUL_1_STEP(0)
    }
#undef LMMP_SUBMUL_1_STEP
    return d;
}

void lmmp_mullo_basecase_(mp_ptr restrict dst, mp_srcptr restrict numa, mp_srcptr restrict numb, mp_size_t n) {
    mp_limb_t h;
    h = numa[0] * numb[n - 1];
    if (n != 1) {
        mp_size_t i;
        mp_limb_t b0;

        b0 = *numb++;
        h += numa[n - 1] * b0 + lmmp_mul_1_(dst, numa, n - 1, b0);
        dst++;

        for (i = n - 2; i > 0; i--) {
            b0 = *numb++;
            h += numa[i] * b0 + lmmp_addmul_1_(dst, numa, i, b0);
            dst++;
        }
    }

    dst[0] = h;
}

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

#include "../../../../include/lmmp/lmmpn.h"

/*
    汇编不可用时的通用 C 实现（eqsep 重叠安全：每组加载先于同组存储）。
    汇编构建使用 asm/{x64,arm64}/add_n_sub_n.S。
    差链与 sub_n_ 同样补码化: d = a + ~b + k, k0 = 1, 借位 = 1 - k，
    借用编译器对 3 项 128 位加法的并行 add/adc 分解；~b 须先读入局部量，
    避免被 clang 向量化破坏进位链模式匹配。
*/

mp_limb_t lmmp_add_n_sub_n_(mp_ptr dsta, mp_ptr dstb, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i = 0;
    mp_limb_t cy = 0;
    mp_limb_t k = 1;

    for (; i + 4 <= n; i += 4) {
        mp_limb_t a0 = numa[i + 0], a1 = numa[i + 1], a2 = numa[i + 2], a3 = numa[i + 3];
        mp_limb_t b0 = numb[i + 0], b1 = numb[i + 1], b2 = numb[i + 2], b3 = numb[i + 3];
        mp_limb_t nb0 = ~b0, nb1 = ~b1, nb2 = ~b2, nb3 = ~b3;

        __uint128_t s0 = (__uint128_t)a0 + b0 + cy;
        __uint128_t s1 = (__uint128_t)a1 + b1 + (mp_limb_t)(s0 >> 64);
        __uint128_t s2 = (__uint128_t)a2 + b2 + (mp_limb_t)(s1 >> 64);
        __uint128_t s3 = (__uint128_t)a3 + b3 + (mp_limb_t)(s2 >> 64);
        cy = (mp_limb_t)(s3 >> 64);

        __uint128_t t0 = (__uint128_t)a0 + nb0 + k;
        __uint128_t t1 = (__uint128_t)a1 + nb1 + (mp_limb_t)(t0 >> 64);
        __uint128_t t2 = (__uint128_t)a2 + nb2 + (mp_limb_t)(t1 >> 64);
        __uint128_t t3 = (__uint128_t)a3 + nb3 + (mp_limb_t)(t2 >> 64);
        k = (mp_limb_t)(t3 >> 64);

        dsta[i + 0] = (mp_limb_t)s0;
        dsta[i + 1] = (mp_limb_t)s1;
        dsta[i + 2] = (mp_limb_t)s2;
        dsta[i + 3] = (mp_limb_t)s3;
        dstb[i + 0] = (mp_limb_t)t0;
        dstb[i + 1] = (mp_limb_t)t1;
        dstb[i + 2] = (mp_limb_t)t2;
        dstb[i + 3] = (mp_limb_t)t3;
    }
    for (; i < n; i++) {
        mp_limb_t a = numa[i], b = numb[i], nb = ~b;
        __uint128_t s = (__uint128_t)a + b + cy;
        __uint128_t t = (__uint128_t)a + nb + k;
        dsta[i] = (mp_limb_t)s;
        cy = (mp_limb_t)(s >> 64);
        dstb[i] = (mp_limb_t)t;
        k = (mp_limb_t)(t >> 64);
    }

    return 2 * cy + (1 - k);
}

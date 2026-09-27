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


#if defined(__SIZEOF_INT128__)

mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    /*
        a - b - c = a + ~b + (1 - c), 借位补码化后与 add_nc_ 完全同构，
        借用编译器对 3 项 128 位加法的并行 add/adc 分解，消除减法借位
        提取(shr+neg)在迭代间串行链上的开销。~b 须先读入局部量：
        表达式内直接 ~numb[i] 会被 clang 向量化(成对的 movdqu/pxor)，
        破坏进位链模式匹配，实测劣化约 2 倍。
    */
    mp_size_t i = 0;
    mp_limb_t cy = 1 - c;

    for (; i + 4 <= n; i += 4) {
        mp_limb_t nb0 = ~numb[i + 0], nb1 = ~numb[i + 1], nb2 = ~numb[i + 2], nb3 = ~numb[i + 3];
        __uint128_t s0 = (__uint128_t)numa[i + 0] + nb0 + cy;
        __uint128_t s1 = (__uint128_t)numa[i + 1] + nb1 + (mp_limb_t)(s0 >> 64);
        __uint128_t s2 = (__uint128_t)numa[i + 2] + nb2 + (mp_limb_t)(s1 >> 64);
        __uint128_t s3 = (__uint128_t)numa[i + 3] + nb3 + (mp_limb_t)(s2 >> 64);
        dst[i + 0] = (mp_limb_t)s0;
        dst[i + 1] = (mp_limb_t)s1;
        dst[i + 2] = (mp_limb_t)s2;
        dst[i + 3] = (mp_limb_t)s3;
        cy = (mp_limb_t)(s3 >> 64);
    }
    for (; i < n; i++) {
        mp_limb_t nb = ~numb[i];
        __uint128_t s = (__uint128_t)numa[i] + nb + cy;
        dst[i] = (mp_limb_t)s;
        cy = (mp_limb_t)(s >> 64);
    }

    return 1 - cy;
}

mp_limb_t lmmp_sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    return lmmp_sub_nc_(dst, numa, numb, n, 0);
}

#else /* !__SIZEOF_INT128__ 朴素参考实现 */

mp_limb_t lmmp_sub_nc_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t c) {
    mp_size_t i = 0;
    mp_limb_t cy = c;

    for (; i < n; i++) {
        mp_limb_t a, b;
        a = numa[i];
        b = numb[i];
        b += cy;
        cy = (b < cy);
        cy += (a < b);
        dst[i] = a - b;
    }

    return cy;
}

mp_limb_t lmmp_sub_n_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    mp_size_t i = 0;
    mp_limb_t cy = 0;

    for (; i < n; i++) {
        mp_limb_t a, b;
        a = numa[i];
        b = numb[i];
        b += cy;
        cy = (b < cy);
        cy += (a < b);
        dst[i] = a - b;
    }

    return cy;
}

#endif /* __SIZEOF_INT128__ */

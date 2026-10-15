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

#include "../../../include/lmmp/lmmpn.h"


int lmmp_cmp_(mp_srcptr numa, mp_srcptr numb, mp_size_t n) {
    lmmp_param_assert(n > 0);
    lmmp_param_assert(numa != NULL);
    lmmp_param_assert(numb != NULL);
    /* 顶部剥皮 r = 4,1,2,3 个 limb（依 n mod 4，0 取 4）：短行程早退窗口，
       余段长度恰为 4 的倍数，下方 4-limb 步进无需标量尾与运行时版本化 */
    mp_size_t r = n & 3;
    if (r == 0) r = 4;
    for (mp_size_t k = 0; k < r; k++) {
        mp_limb_t x = numa[n - 1 - k];
        mp_limb_t y = numb[n - 1 - k];
        if (x != y)
            return (x > y ? 1 : -1);
    }
    /* 余段自顶向下按 4 limb 一组：异或折叠判差，有差组内自顶定位 */
    for (mp_size_t i = n - r; i != 0;) {
        i -= 4;
        mp_limb_t d = (numa[i + 0] ^ numb[i + 0]) | (numa[i + 1] ^ numb[i + 1]) |
                      (numa[i + 2] ^ numb[i + 2]) | (numa[i + 3] ^ numb[i + 3]);
        if (d) {
            for (mp_size_t j = i + 3; j >= i; j--) {
                if (numa[j] != numb[j])
                    return (numa[j] > numb[j] ? 1 : -1);
            }
        }
    }
    return 0;
}

int lmmp_zero_q_(mp_srcptr p, mp_size_t n) {
    lmmp_param_assert(n > 0);
    lmmp_param_assert(p != NULL);
    /* 同 lmmp_cmp_ 的剥皮结构 */
    mp_size_t r = n & 3;
    if (r == 0) r = 4;
    for (mp_size_t k = 0; k < r; k++) {
        if (p[n - 1 - k] != 0)
            return 0;
    }
    /* 余段按 4 limb 一组，或折叠非零检测 */
    for (mp_size_t i = n - r; i != 0;) {
        i -= 4;
        if ((p[i] | p[i + 1] | p[i + 2] | p[i + 3]) != 0)
            return 0;
    }
    return 1;
}

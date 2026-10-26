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

/*
    fft/ifft 蝶形数据面融合内核 lmmp_fft_bfy_fused_ / lmmp_ifft_bfy_fused_ 的 C 参考实现。

    x64 汇编构建下符号由 asm/x64/fft_fuse.S 提供（本文件被构建系统排除）；
    GENERIC 与 arm64 汇编构建走此 C 实现（arm64 NEON 版待移植，由 CMake 补入）。

    顺序流式访问（每 limb 恰好读写一次），不做 PART_SIZE 分块。
    差链补码化同 add_n_sub_n_: d = a + ~t + k, k0 = 1 - b, 借位 = 1 - k。
*/

#include "../../../../include/lmmp/impl/fft_ssa.h"

mp_limb_t lmmp_ifft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb,
                               mp_size_t l, mp_size_t w, mp_size_t shr) {
    const unsigned rshr = LIMB_BITS - shr;
    mp_limb_t acy = 0, bcy = 0;  // acy=numa 链, bcy=numc 链（各段进出解包/重包）
    mp_limb_t cur;

    // 扫描1: numc[l-w+i] = numa[l-w+i] + t, numa[l-w+i] -= t（和→numc 即 bcy, 差→numa 即 acy）
    cur = numb[0];
    {
        mp_limb_t ca = bcy, k = 1 - acy;
        for (mp_size_t i = 0; i < w; i++) {
            mp_limb_t nx = (i + 1 < w) ? numb[i + 1] : numb[w];  // 顶 limb 的 hi 来自 numb[w]
            mp_limb_t t = (cur >> shr) | (nx << rshr);
            cur = nx;
            mp_limb_t a = numa[l - w + i];

            __uint128_t s = (__uint128_t)a + t + ca;
            __uint128_t d = (__uint128_t)a + ~t + k;
            numc[l - w + i] = (mp_limb_t)s;
            numa[l - w + i] = (mp_limb_t)d;
            ca = (mp_limb_t)(s >> 64);
            k = (mp_limb_t)(d >> 64);
        }
        bcy = ca;
        acy = 1 - k;
    }

    // 扫描2: numc[i] = numa[i] - t', numa[i] += t'（和→numa 即 acy, 差→numc 即 bcy）
    cur = numb[w];
    {
        mp_limb_t ca = acy, k = 1 - bcy;
        for (mp_size_t i = 0; i < l - w; i++) {
            mp_limb_t nx = (i + 1 < l - w) ? numb[w + i + 1] : numb[l];  // 顶 limb 的 hi 来自 numb[l]
            mp_limb_t t = (cur >> shr) | (nx << rshr);
            cur = nx;
            mp_limb_t a = numa[i];

            __uint128_t s = (__uint128_t)a + t + ca;
            __uint128_t d = (__uint128_t)a + ~t + k;
            numa[i] = (mp_limb_t)s;
            numc[i] = (mp_limb_t)d;
            ca = (mp_limb_t)(s >> 64);
            k = (mp_limb_t)(d >> 64);
        }
        acy = ca;
        bcy = 1 - k;
    }

    return (acy << 1) | bcy;
}

void lmmp_fft_bfy_fused_(mp_ptr numc, mp_ptr numa, mp_ptr numb, mp_size_t l, mp_size_t w,
                         mp_size_t shl, mp_limb_t* out) {
    const unsigned rshl = LIMB_BITS - shl;
    mp_limb_t ca = 0, b = 0, prev = 0;

    // 扫描1: numc[w+j] = (numa[j]-numb[j])<<shl 接链, numa[j] += numb[j]
    for (mp_size_t j = 0; j < l - w; j++) {
        mp_limb_t x = numa[j], y = numb[j];

        mp_limb_t u = x - y;
        mp_limb_t d = u - b;
        b = (u > x) + (d > u);

        __uint128_t s = (__uint128_t)x + y + ca;
        numa[j] = (mp_limb_t)s;
        ca = (mp_limb_t)(s >> 64);

        numc[w + j] = (d << shl) | (prev >> rshl);
        prev = d;
    }

    // 中点 ch 分解（acyo 链续, scyo/shl 链重置）
    {
        mp_limb_t shlcy = prev >> rshl;
        mp_limb_t adj = b << shl;
        out[0] = (shlcy >= adj) ? shlcy - adj : 0;
        out[1] = (shlcy >= adj) ? 0 : adj - shlcy;
    }
    b = 0;
    prev = 0;

    // 扫描2: numc[j] = (numb[l-w+j]-numa[l-w+j])<<shl 接链, numa[l-w+j] += numb[l-w+j]
    for (mp_size_t j = 0; j < w; j++) {
        mp_limb_t x = numb[l - w + j], y = numa[l - w + j];

        mp_limb_t u = x - y;
        mp_limb_t d = u - b;
        b = (u > x) + (d > u);

        __uint128_t s = (__uint128_t)x + y + ca;
        numa[l - w + j] = (mp_limb_t)s;
        ca = (mp_limb_t)(s >> 64);

        numc[j] = (d << shl) | (prev >> rshl);
        prev = d;
    }

    out[2] = prev >> rshl;
    out[3] = (ca << 1) | b;
}

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

#include "../../../include/lmmp/impl/rand_state.h"


void lmmp_xoshiro256ppx8_srandom(xoshiro256ppx8_state* rng, mp_limb_t seed) {
    lmmp_param_assert(rng != NULL);

    const mp_limb_t C = 0x9e3779b97f4a7c15ULL;

    for (int j = 0; j < 8; j++) {
        rng->s0[j] = mix64(seed + (4 * j + 0) * C);
        rng->s1[j] = mix64(seed + (4 * j + 1) * C);
        rng->s2[j] = mix64(seed + (4 * j + 2) * C);
        rng->s3[j] = mix64(seed + (4 * j + 3) * C);
    }
    // 同单流：mix64 双射 + 互异输入 ⟹ 每流至多一个零 limb，无全零流
}

void lmmp_xoshiro256ppx8_fill_(mp_ptr restrict dst, mp_size_t n, xoshiro256ppx8_state* restrict rng) {
    lmmp_param_assert(rng != NULL);
    if (n == 0) return;

    /* 状态字装载进命名局部（s0 流 a*，s1 流 b*，s2 流 c*，s3 流 d*；
       下标即流号 j），主循环 8 lane 连续同构语句组 = SLP 最简形态 */
    mp_limb_t a0 = rng->s0[0], a1 = rng->s0[1], a2 = rng->s0[2], a3 = rng->s0[3];
    mp_limb_t a4 = rng->s0[4], a5 = rng->s0[5], a6 = rng->s0[6], a7 = rng->s0[7];
    mp_limb_t b0 = rng->s1[0], b1 = rng->s1[1], b2 = rng->s1[2], b3 = rng->s1[3];
    mp_limb_t b4 = rng->s1[4], b5 = rng->s1[5], b6 = rng->s1[6], b7 = rng->s1[7];
    mp_limb_t c0 = rng->s2[0], c1 = rng->s2[1], c2 = rng->s2[2], c3 = rng->s2[3];
    mp_limb_t c4 = rng->s2[4], c5 = rng->s2[5], c6 = rng->s2[6], c7 = rng->s2[7];
    mp_limb_t d0 = rng->s3[0], d1 = rng->s3[1], d2 = rng->s3[2], d3 = rng->s3[3];
    mp_limb_t d4 = rng->s3[4], d5 = rng->s3[5], d6 = rng->s3[6], d7 = rng->s3[7];

    mp_size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        mp_limb_t r0 = rotl(a0 + d0, 23) + a0;
        mp_limb_t r1 = rotl(a1 + d1, 23) + a1;
        mp_limb_t r2 = rotl(a2 + d2, 23) + a2;
        mp_limb_t r3 = rotl(a3 + d3, 23) + a3;
        mp_limb_t r4 = rotl(a4 + d4, 23) + a4;
        mp_limb_t r5 = rotl(a5 + d5, 23) + a5;
        mp_limb_t r6 = rotl(a6 + d6, 23) + a6;
        mp_limb_t r7 = rotl(a7 + d7, 23) + a7;
        mp_limb_t t0 = b0 << 17;
        mp_limb_t t1 = b1 << 17;
        mp_limb_t t2 = b2 << 17;
        mp_limb_t t3 = b3 << 17;
        mp_limb_t t4 = b4 << 17;
        mp_limb_t t5 = b5 << 17;
        mp_limb_t t6 = b6 << 17;
        mp_limb_t t7 = b7 << 17;
        c0 ^= a0;
        c1 ^= a1;
        c2 ^= a2;
        c3 ^= a3;
        c4 ^= a4;
        c5 ^= a5;
        c6 ^= a6;
        c7 ^= a7;
        d0 ^= b0;
        d1 ^= b1;
        d2 ^= b2;
        d3 ^= b3;
        d4 ^= b4;
        d5 ^= b5;
        d6 ^= b6;
        d7 ^= b7;
        b0 ^= c0;
        b1 ^= c1;
        b2 ^= c2;
        b3 ^= c3;
        b4 ^= c4;
        b5 ^= c5;
        b6 ^= c6;
        b7 ^= c7;
        a0 ^= d0;
        a1 ^= d1;
        a2 ^= d2;
        a3 ^= d3;
        a4 ^= d4;
        a5 ^= d5;
        a6 ^= d6;
        a7 ^= d7;
        c0 ^= t0;
        c1 ^= t1;
        c2 ^= t2;
        c3 ^= t3;
        c4 ^= t4;
        c5 ^= t5;
        c6 ^= t6;
        c7 ^= t7;
        d0 = rotl(d0, 45);
        d1 = rotl(d1, 45);
        d2 = rotl(d2, 45);
        d3 = rotl(d3, 45);
        d4 = rotl(d4, 45);
        d5 = rotl(d5, 45);
        d6 = rotl(d6, 45);
        d7 = rotl(d7, 45);
        dst[i + 0] = r0;
        dst[i + 1] = r1;
        dst[i + 2] = r2;
        dst[i + 3] = r3;
        dst[i + 4] = r4;
        dst[i + 5] = r5;
        dst[i + 6] = r6;
        dst[i + 7] = r7;
    }

    rng->s0[0] = a0;
    rng->s0[1] = a1;
    rng->s0[2] = a2;
    rng->s0[3] = a3;
    rng->s0[4] = a4;
    rng->s0[5] = a5;
    rng->s0[6] = a6;
    rng->s0[7] = a7;
    rng->s1[0] = b0;
    rng->s1[1] = b1;
    rng->s1[2] = b2;
    rng->s1[3] = b3;
    rng->s1[4] = b4;
    rng->s1[5] = b5;
    rng->s1[6] = b6;
    rng->s1[7] = b7;
    rng->s2[0] = c0;
    rng->s2[1] = c1;
    rng->s2[2] = c2;
    rng->s2[3] = c3;
    rng->s2[4] = c4;
    rng->s2[5] = c5;
    rng->s2[6] = c6;
    rng->s2[7] = c7;
    rng->s3[0] = d0;
    rng->s3[1] = d1;
    rng->s3[2] = d2;
    rng->s3[3] = d3;
    rng->s3[4] = d4;
    rng->s3[5] = d5;
    rng->s3[6] = d6;
    rng->s3[7] = d7;

    /* 尾部（n%8）：多产出一块仅取所需（各流锁步推进的延续；块体与
       主循环同构展开，尾部冷路径无需向量化） */
    if (i < n) {
        mp_limb_t r0 = rotl(a0 + d0, 23) + a0;
        mp_limb_t r1 = rotl(a1 + d1, 23) + a1;
        mp_limb_t r2 = rotl(a2 + d2, 23) + a2;
        mp_limb_t r3 = rotl(a3 + d3, 23) + a3;
        mp_limb_t r4 = rotl(a4 + d4, 23) + a4;
        mp_limb_t r5 = rotl(a5 + d5, 23) + a5;
        mp_limb_t r6 = rotl(a6 + d6, 23) + a6;
        mp_limb_t r7 = rotl(a7 + d7, 23) + a7;
        mp_limb_t t0 = b0 << 17;
        mp_limb_t t1 = b1 << 17;
        mp_limb_t t2 = b2 << 17;
        mp_limb_t t3 = b3 << 17;
        mp_limb_t t4 = b4 << 17;
        mp_limb_t t5 = b5 << 17;
        mp_limb_t t6 = b6 << 17;
        mp_limb_t t7 = b7 << 17;
        c0 ^= a0;
        c1 ^= a1;
        c2 ^= a2;
        c3 ^= a3;
        c4 ^= a4;
        c5 ^= a5;
        c6 ^= a6;
        c7 ^= a7;
        d0 ^= b0;
        d1 ^= b1;
        d2 ^= b2;
        d3 ^= b3;
        d4 ^= b4;
        d5 ^= b5;
        d6 ^= b6;
        d7 ^= b7;
        b0 ^= c0;
        b1 ^= c1;
        b2 ^= c2;
        b3 ^= c3;
        b4 ^= c4;
        b5 ^= c5;
        b6 ^= c6;
        b7 ^= c7;
        a0 ^= d0;
        a1 ^= d1;
        a2 ^= d2;
        a3 ^= d3;
        a4 ^= d4;
        a5 ^= d5;
        a6 ^= d6;
        a7 ^= d7;
        c0 ^= t0;
        c1 ^= t1;
        c2 ^= t2;
        c3 ^= t3;
        c4 ^= t4;
        c5 ^= t5;
        c6 ^= t6;
        c7 ^= t7;
        mp_limb_t rr[8] = {r0, r1, r2, r3, r4, r5, r6, r7};
        for (mp_size_t k = 0; i + k < n; k++) dst[i + k] = rr[k];
    }
}

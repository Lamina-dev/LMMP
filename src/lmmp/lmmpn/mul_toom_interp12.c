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

#include "../../../include/lmmp/impl/divexact.h"
#include "../../../include/lmmp/impl/toom_interp.h"


#define DO_subrsh(dst, nd, src, ns, s)                       \
    do {                                                     \
        mp_limb_t __cy;                                      \
        lmmp_dec_1((dst), (src)[0] >> (s));                  \
        __cy = lmmp_submul_1_((dst), (src) + 1, (ns) - 1, (mp_limb_t)1 << (LIMB_BITS - (s))); \
        lmmp_dec_1((dst) + (ns) - 1, __cy);                  \
    } while (0)

#define BINVERT_9_ ((mp_limb_t)0x8E38E38E38E38E39)
#define BINVERT_255_ ((mp_limb_t)0xFEFEFEFEFEFEFEFF)
#define BINVERT_2835_ ((mp_limb_t)0x938CC70553E3771B)
#define BINVERT_42525_ ((mp_limb_t)0xE7B40D449F314C35)

void lmmp_toom_interp12_(
    mp_ptr dst, mp_ptr r1, mp_ptr r3, mp_ptr r5, mp_size_t n, mp_size_t spt, int half, mp_ptr tp
) {
    mp_limb_t cy, cy2;
    mp_size_t n3, n3p1;
    n3 = 3 * n;
    n3p1 = n3 + 1;

    lmmp_param_assert(n > 0);
    lmmp_param_assert(spt > 0);
    lmmp_param_assert(spt <= 2 * n);

#define r4 (dst + n3)     /* 3n+1 */
#define r2 (dst + 7 * n)  /* 3n+1 */
#define r0 (dst + 11 * n) /* spt <= 2n */

    /* 插值序列（值引用为混合后的和/差半边）：
       r? <- (r? - r0*x)/d 的消顶部项 + 交叉消元 + 精确除，最后重组 */

    if (half != 0) {
        cy = lmmp_sub_n_(r3, r3, r0, spt);
        lmmp_dec_1(r3 + spt, cy);

        cy = lmmp_submul_1_(r2, r0, spt, (mp_limb_t)1 << 10);
        lmmp_dec_1(r2 + spt, cy);
        DO_subrsh(r5, n3p1, r0, spt, 2);

        cy = lmmp_submul_1_(r1, r0, spt, (mp_limb_t)1 << 20);
        lmmp_dec_1(r1 + spt, cy);
        DO_subrsh(r4, n3p1, r0, spt, 4);
    }

    r4[n3] -= lmmp_submul_1_(r4 + n, dst, 2 * n, (mp_limb_t)1 << 20);
    DO_subrsh(r1 + n, 2 * n + 1, dst, 2 * n, 4);

    lmmp_add_n_sub_n_(r1, r4, r4, r1, n3p1);

    r5[n3] -= lmmp_submul_1_(r5 + n, dst, 2 * n, (mp_limb_t)1 << 10);
    DO_subrsh(r2 + n, 2 * n + 1, dst, 2 * n, 2);

    lmmp_add_n_sub_n_(r2, r5, r5, r2, n3p1);

    r3[n3] -= lmmp_sub_n_(r3 + n, r3 + n, dst, 2 * n);

    lmmp_submul_1_(r4, r5, n3p1, 257);
    /* 随后除以 2835*4，操作数可能为负（补码） */
    lmmp_divexact_odd_pi_(r4, r4, n3p1, 2835, BINVERT_2835_, 2);
    if ((r4[n3] & (LIMB_MAX << (LIMB_BITS - 3))) != 0)
        r4[n3] |= LIMB_MAX << (LIMB_BITS - 2);

    lmmp_addmul_1_(r5, r4, n3p1, 60);
    lmmp_divexact_odd_pi_(r5, r5, n3p1, 255, BINVERT_255_, 0);

    cy = lmmp_submul_1_(r2, r3, n3p1, (mp_limb_t)1 << 5);
    lmmp_debug_assert(cy == 0);

    cy = lmmp_submul_1_(r1, r2, n3p1, 100);
    lmmp_debug_assert(cy == 0);
    cy = lmmp_submul_1_(r1, r3, n3p1, (mp_limb_t)1 << 9);
    lmmp_debug_assert(cy == 0);
    lmmp_divexact_odd_pi_(r1, r1, n3p1, 42525, BINVERT_42525_, 0);

    cy = lmmp_submul_1_(r2, r1, n3p1, 225);
    lmmp_debug_assert(cy == 0);
    lmmp_divexact_odd_pi_(r2, r2, n3p1, 9, BINVERT_9_, 2);

    cy = lmmp_sub_n_(r3, r3, r2, n3p1);
    lmmp_debug_assert(cy == 0);

    lmmp_shr1sub_n_(r4, r2, r4, n3p1);
    r4[n3p1 - 1] &= LIMB_MAX >> 1;
    cy = lmmp_sub_n_(r2, r2, r4, n3p1);
    lmmp_debug_assert(cy == 0);

    lmmp_shr1add_n_(r5, r5, r1, n3p1);
    r5[n3p1 - 1] &= LIMB_MAX >> 1;

    /* 最后两步插值（可并入重组） */
    cy = lmmp_sub_n_(r3, r3, r1, n3p1);
    lmmp_debug_assert(cy == 0);
    cy = lmmp_sub_n_(r1, r1, r5, n3p1);
    lmmp_debug_assert(cy == 0);

    (void)tp; /* 现有原语集（submul/addmul/原生 add_n_sub_n）不消耗临时区 */

    /***************************** 重组 *******************************/
    /*
      dst 操作前：
      |M r0|L r0|___||H r2|M r2|L r2|___||H r4|M r4|L r4|____|H r8|L r8|dst
      求和方案：
      |__12|n_11|n_10|n__9|n__8|n__7|n__6|n__5|n__4|n__3|n__2|n___|n___|dst
      |M r0|L r0|___||H r2|M r2|L r2|___||H r4|M r4|L r4|____|H_r8|L_r8|dst
          ||H r1|M r1|L r1|   ||H r3|M r3|L r3|   ||H_r5|M_r5|L_r5|
    */

    cy = lmmp_add_n_(dst + n, dst + n, r5, n);
    cy = lmmp_add_1_(dst + 2 * n, r5 + n, n, cy);
    cy = r5[n3] + lmmp_add_nc_(dst + n3, dst + n3, r5 + 2 * n, n, cy);
    lmmp_inc_1(dst + n3 + n, cy);

    dst[2 * n3] += lmmp_add_n_(dst + 5 * n, dst + 5 * n, r3, n);
    cy = lmmp_add_1_(dst + 2 * n3, r3 + n, n, dst[2 * n3]);
    cy = r3[n3] + lmmp_add_nc_(dst + 7 * n, dst + 7 * n, r3 + 2 * n, n, cy);
    lmmp_inc_1(dst + 8 * n, cy);

    dst[10 * n] += lmmp_add_n_(dst + 9 * n, dst + 9 * n, r1, n);
    if (half) {
        cy = lmmp_add_1_(dst + 10 * n, r1 + n, n, dst[10 * n]);
        if (spt > n) {
            cy = r1[n3] + lmmp_add_nc_(dst + 11 * n, dst + 11 * n, r1 + 2 * n, n, cy);
            lmmp_inc_1(dst + 4 * n3, cy);
        } else {
            cy2 = lmmp_add_nc_(dst + 11 * n, dst + 11 * n, r1 + 2 * n, spt, cy);
            lmmp_debug_assert(cy2 == 0);
        }
    } else {
        cy = lmmp_add_1_(dst + 10 * n, r1 + n, spt, dst[10 * n]);
        lmmp_debug_assert(cy == 0);
    }

#undef r4
#undef r2
#undef r0
}

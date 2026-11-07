//  Copyright (C) 2026 HJimmyK (Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as
//  published by the Free Software Foundation; either version 3 of
//  the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, see
//  <https://www.gnu.org/licenses/>.
//
//  SPDX-License-Identifier: LGPL-3.0-or-later

//
// mp_limb_t lmmp_add_n_sub_n_(mp_ptr dsta, mp_ptr dstb, mp_srcptr numa, mp_srcptr numb,
//                             mp_size_t n);
//
//   dsta=x0, dstb=x1, numa=x2, numb=x3, n=x4

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_add_n_sub_n_
// ======================================
.globl ASM_GSYM(lmmp_add_n_sub_n_)
.p2align 2
ASM_GSYM(lmmp_add_n_sub_n_):
    mov     x12, #0                 // cAdd = 0
    mov     x13, #0                 // bSub = 0
    lsr     x17, x4, #2             // x17 = n/4
    and     x4, x4, #3              // x4 = n mod 4
    cbz     x17, Lanss_tail

.p2align 4
Lanss_top:                          // main loop, 4 limbs
    ldp     x5, x6, [x2], #16       // a0, a1
    ldp     x7, x8, [x2], #16       // a2, a3
    ldp     x9, x10, [x3], #16      // b0, b1
    ldp     x11, x14, [x3], #16     // b2, b3

    cmp     x12, #1                 // C = cAdd
    adcs    x15, x5, x9             // s0 = a0 + b0 + cAdd
    adcs    x16, x6, x10            // s1 = a1 + b1 + c0
    stp     x15, x16, [x0], #16
    adcs    x15, x7, x11            // s2 = a2 + b2 + c1
    adcs    x16, x8, x14            // s3 = a3 + b3 + c2
    stp     x15, x16, [x0], #16
    cset    x12, cs                 // cAdd' = carry(s3)

    cmp     xzr, x13                // C = !bSub
    sbcs    x5, x5, x9              // d0 = a0 - b0 - bSub
    sbcs    x6, x6, x10             // d1 = a1 - b1 - b0
    stp     x5, x6, [x1], #16
    sbcs    x7, x7, x11             // d2 = a2 - b2 - b1
    sbcs    x8, x8, x14             // d3 = a3 - b3 - b2
    stp     x7, x8, [x1], #16
    cset    x13, cc                 // bSub' = borrow(d3)

    subs    x17, x17, #1
    cbnz    x17, Lanss_top

Lanss_tail:                         // tail: 0..3 limbs
    cbz     x4, Lanss_end
    ldr     x5, [x2], #8            // a
    ldr     x9, [x3], #8            // b
    cmp     x12, #1                 // C = cAdd
    adcs    x15, x5, x9             // s = a + b + cAdd
    cset    x12, cs
    cmp     xzr, x13                // C = !bSub
    sbcs    x5, x5, x9              // d = a - b - bSub
    cset    x13, cc
    str     x15, [x0], #8
    str     x5, [x1], #8
    subs    x4, x4, #1
    b.ne    Lanss_tail

Lanss_end:
    add     x0, x13, x12, lsl #1    // ret = 2*cAdd + bSub
    ret

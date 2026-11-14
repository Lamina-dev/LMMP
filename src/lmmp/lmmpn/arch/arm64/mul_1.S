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
// mp_limb_t lmmp_mul_1_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_limb_t x);
//
//   dst=x0, numa=x1, na=x2, x=x3

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_mul_1_
// ======================================
.globl ASM_GSYM(lmmp_mul_1_)
.p2align 2
ASM_GSYM(lmmp_mul_1_):
    adds    x4, xzr, xzr                // carry = 0, C = 0

Lm1_com:
    lsr     x17, x2, #2                 // x17 = na >> 2
    tbnz    x2, #0, Lm1_bx1             // n mod 2 == 1

Lm1_bx0:
    mov     x11, x4                     // entry carry
    tbz     x2, #1, Lm1_b00             // n mod 4 == 0

Lm1_b10:                                // n mod 4 == 2
    ldp     x4, x5, [x1]                // a0, a1
    mul     x8, x4, x3                  // lo0
    umulh   x10, x4, x3                 // hi0
    cbz     x17, Lm1_2                  // n == 2, ret
    ldp     x6, x7, [x1, #16]!          // prefetch a2, a3
    mul     x9, x5, x3                  // lo1
    b       Lm1_mid-8                   // enter mid loop

Lm1_2:                                  // n == 2 tail
    mul     x9, x5, x3                  // lo1
    b       Lm1_2e

Lm1_bx1:                                // leading single limb
    ldr     x7, [x1], #8
    mul     x9, x7, x3                  // lo0
    umulh   x11, x7, x3                 // hi0
    adds    x9, x9, x4                  // s0 = lo0 + carry
    str     x9, [x0], #8
    tbnz    x2, #1, Lm1_b10             // n mod 4 == 2

Lm1_b01:
    cbz     x17, Lm1_1                  // n == 1, ret

Lm1_b00:                                // n mod 4 == 0
    ldp     x6, x7, [x1]                // a0, a1
    mul     x8, x6, x3                  // lo0
    umulh   x10, x6, x3                 // hi0
    ldp     x4, x5, [x1, #16]           // a2, a3
    mul     x9, x7, x3                  // lo1
    adcs    x12, x8, x11                // s0 = lo0 + hi_prev + C
    umulh   x11, x7, x3                 // hi1
    add     x0, x0, #16
    sub     x17, x17, #1
    cbz     x17, Lm1_end                // last pair only
.p2align 4

Lm1_top:                                // main loop (upper)
    mul     x8, x4, x3                  // lo2
    ldp     x6, x7, [x1, #32]!          // next pair
    adcs    x13, x9, x10                // s1 = lo1 + hi0 + C
    umulh   x10, x4, x3                 // hi2
    mul     x9, x5, x3                  // lo3
    stp     x12, x13, [x0, #-16]        // store s0, s1
    adcs    x12, x8, x11                // s2 = lo2 + hi1 + C
    umulh   x11, x5, x3                 // hi3

Lm1_mid:                                // main loop (lower)
    mul     x8, x6, x3                  // lo0
    ldp     x4, x5, [x1, #16]           // next pair
    adcs    x13, x9, x10                // s3 = lo3 + hi2 + C
    umulh   x10, x6, x3                 // hi0
    mul     x9, x7, x3                  // lo1
    stp     x12, x13, [x0], #32         // store s2, s3
    adcs    x12, x8, x11                // s0 = lo0 + hi3 + C
    umulh   x11, x7, x3                 // hi1
    sub     x17, x17, #1
    cbnz    x17, Lm1_top

Lm1_end:                                // tail: last prefetched pair
    mul     x8, x4, x3                  // lo0
    adcs    x13, x9, x10                // s1 = lo1 + hi0 + C
    umulh   x10, x4, x3                 // hi0
    mul     x9, x5, x3                  // lo1
    stp     x12, x13, [x0, #-16]        // store s0, s1

Lm1_2e:                                 // 2-limb tail
    adcs    x12, x8, x11                // s0 = lo0 + hi_prev + C
    umulh   x11, x5, x3                 // hi1
    adcs    x13, x9, x10                // s1 = lo1 + hi0 + C
    stp     x12, x13, [x0]

Lm1_1:
    adc     x0, x11, xzr                // ret hi_last + C
    ret

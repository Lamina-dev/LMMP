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
// mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c);
// mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr);
//
//   dst=x0, numa=x1, na=x2, shr=x3, c=x4 (only c_)

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_shr_c_
// ======================================
.globl ASM_GSYM(lmmp_shr_c_)
.p2align 2
ASM_GSYM(lmmp_shr_c_):

Lshr_ent:
    cbz     x3, Lshr_copy            // shr == 0, copy
    mov     x5, #64
    sub     x5, x5, x3               // x5 = s = 64 - shr
    neg     x7, x3                   // -shr
    dup     v0.2d, x7                // -k
    dup     v1.2d, x5                // +s
    ldr     x6, [x1]                 // a[0]
    lsl     x6, x6, x5               // ret = a[0] << s
    tbz     x2, #0, Lshr_even        // n even

    // n odd: compute top limb, store after all pairs
    add     x10, x1, x2, lsl #3
    ldr     x11, [x10, #-8]          // a[n-1]
    lsl     x7, x11, x5
    lsr     x11, x11, x3
    orr     x11, x11, x4             // delayed store value
    dup     v2.2d, x7                // fin = a[n-1] << s
    b       Lshr_pro

Lshr_even:
    dup     v2.2d, x4                // fin = c

Lshr_pro:
    lsr     x8, x2, #1               // P = pairs
    cbz     x8, Lshr_done            // n == 1
    ldr     q5, [x1], #16            // p0
    ushl    v4.2d, v5.2d, v1.2d      // LA
    ushl    v3.2d, v5.2d, v0.2d      // RA
    sub     x9, x8, #1
    lsr     x9, x9, #1               // full = (P-1)/2
    cbz     x9, Lshr_left
.p2align 4

Lshr_v4:                               // main loop, 2 pairs/iter
    ldr     q5, [x1], #16              // B
    ushl    v6.2d, v5.2d, v1.2d        // LB
    ushl    v7.2d, v5.2d, v0.2d        // RB
    ext     v5.16b, v4.16b, v6.16b, #8
    orr     v5.16b, v3.16b, v5.16b     // outA = RA | mix
    str     q5, [x0], #16
    ldr     q5, [x1], #16              // C (lookahead)
    ushl    v4.2d, v5.2d, v1.2d        // LC (new LA)
    ushl    v3.2d, v5.2d, v0.2d        // RC (new RA)
    ext     v5.16b, v6.16b, v4.16b, #8 // [LB[1], LC[0]]
    orr     v5.16b, v7.16b, v5.16b     // outB = RB | mix
    str     q5, [x0], #16
    subs    x9, x9, #1
    b.ne    Lshr_v4

Lshr_left:
    tbnz    x8, #0, Lshr_fin         // P odd: A is last pair
    // P even: emit one leftover pair, then fin
    ldr     q7, [x1]                 // last pair raw
    ushl    v6.2d, v7.2d, v1.2d      // L_last
    ext     v5.16b, v4.16b, v6.16b, #8
    orr     v5.16b, v3.16b, v5.16b   // out = RA | [LA[1], L_last[0]]
    str     q5, [x0], #16
    ushl    v3.2d, v7.2d, v0.2d      // R_last
    mov     v4.16b, v6.16b           // LA = L_last

Lshr_fin:                            // last pair: out = RA | [LA[1], fin[0]]
    ext     v5.16b, v4.16b, v2.16b, #8
    orr     v5.16b, v3.16b, v5.16b
    str     q5, [x0], #16

Lshr_done:                           // n odd: x0 = dst+n-1 (2P = n-1)
    tbz     x2, #0, Lshr_ret
    str     x11, [x0]                // store delayed top limb
Lshr_ret:
    mov     x0, x6
    ret

// ======================================
// lmmp_shr_
// ======================================
.globl ASM_GSYM(lmmp_shr_)
.p2align 2
ASM_GSYM(lmmp_shr_):
    mov     x4, xzr                  // c = 0
    b       Lshr_ent

Lshr_copy:                           // shr == 0: copy, ret 0
    cmp     x0, x1
    b.eq    Lshr_copy_done
    lsr     x7, x2, #2
    cbz     x7, Lshr_copy_tail
.p2align 4

Lshr_copy_loop:
    ldr     q3, [x1], #16
    str     q3, [x0], #16
    ldr     q4, [x1], #16
    str     q4, [x0], #16
    subs    x7, x7, #1
    b.ne    Lshr_copy_loop

Lshr_copy_tail:
    ands    x7, x2, #3
    b.eq    Lshr_copy_done

Lshr_copy_tail_loop:
    ldr     x6, [x1], #8
    str     x6, [x0], #8
    subs    x7, x7, #1
    b.ne    Lshr_copy_tail_loop

Lshr_copy_done:
    mov     x0, xzr
    ret

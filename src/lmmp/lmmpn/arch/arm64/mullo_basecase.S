//  Copyright (C) 2026 HJimmyK (Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it
//  under the terms of the GNU Lesser General Public License as
//  published by the Free Software Foundation, either version 3 of
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
// void lmmp_mullo_basecase_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   dst=x0, numa=x1, numb=x2, n=x3

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_mullo_basecase_
// ======================================
// x19=dst, x20=numa, x21=n, x22=row len, x23=numb walker, x3=numb
// kernel temps: x4=v0, x5=v1, x12=cy0, x13=cy1, x6/x7/x16/x17=u,
// x8/x9/x10/x11/x14/x15=prod+sums, x0/x1/x2=rp/up/len walkers
.globl ASM_GSYM(lmmp_mullo_basecase_)
.p2align 2
ASM_GSYM(lmmp_mullo_basecase_):
    cmp     x3, #1
    b.ne    Lmo_main

    // n == 1: dst[0] = lo(a0*b0)
    ldr     x8, [x1]
    ldr     x9, [x2]
    mul     x10, x8, x9
    str     x10, [x0]
    ret

Lmo_main:
    stp     x29, x30, [sp, #-64]!    // save regs
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    mov     x19, x0                  // dst
    mov     x20, x1                  // numa
    mov     x21, x3                  // n
    mov     x3, x2                   // numb preserved in x3

    // seed pair (b[0], b[1]): mul over [numa, n], tail dropped
    ldp     x4, x5, [x2]             // v0, v1
    mov     x12, xzr                 // cy0 = 0
    mov     x13, xzr                 // cy1 = 0
    mov     x0, x19                  // rp = dst
    mov     x1, x20                  // up = numa
    mov     x2, x21                  // len = n
    tbz     x2, #0, Lmo_s1
    ldr     x6, [x1], #8             // u_0
    mul     x8, x6, x4               // lo0
    umulh   x9, x6, x4               // hi0
    mul     x10, x6, x5              // lo1
    umulh   x11, x6, x5              // hi1
    adds    x9, x9, x10              // h = hi0 + lo1  (c1 = 0)
    adcs    x11, x11, xzr            // h2 = hi1 + c2
    adds    x8, x8, x12              // s = lo0 + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1

Lmo_s1:
    tbz     x2, #1, Lmo_s2
    ldp     x6, x7, [x1], #16        // u_0, u_1
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s_0
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    str     x8, [x0], #8
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s_1
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    str     x8, [x0], #8

Lmo_s2:
    lsr     x2, x2, #2
    cbz     x2, Lmo_send             // tail stores dropped (truncation)
.p2align 4

Lmo_stop:
    ldp     x6, x7, [x1], #32        // u_0, u_1
    ldp     x16, x17, [x1, #-16]     // u_2, u_3
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s_0
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    mul     x9, x7, x4
    umulh   x10, x7, x4
    mul     x11, x7, x5
    umulh   x14, x7, x5
    adds    x10, x10, x11            // h'
    adcs    x14, x14, xzr            // h2'
    adds    x9, x9, x12              // s_1
    adcs    x12, x10, x13            // cy0
    adcs    x13, x14, xzr            // cy1
    stp     x8, x9, [x0], #16
    mul     x8, x16, x4
    umulh   x9, x16, x4
    mul     x10, x16, x5
    umulh   x11, x16, x5
    adds    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s_2
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    mul     x9, x17, x4
    umulh   x10, x17, x4
    mul     x11, x17, x5
    umulh   x14, x17, x5
    adds    x10, x10, x11            // h'
    adcs    x14, x14, xzr            // h2'
    adds    x9, x9, x12              // s_3
    adcs    x12, x10, x13            // cy0
    adcs    x13, x14, xzr            // cy1
    stp     x8, x9, [x0], #16
    subs    x2, x2, #1
    b.ne    Lmo_stop

Lmo_send:
    sub     x22, x21, #2             // pending pair row len (j = 2)
    add     x23, x3, #16             // numb walker at j = 2
    sub     x0, x0, x22, lsl #3      // rp of pending row (dst + 16)
    sub     x1, x1, x21, lsl #3      // up = numa (seed walked [numa, n])
    cmp     x22, #2
    b.lt    Lmo_left                 // n == 2 done, n == 3 leftover only

    // pair rows j = 2, 4, ...: v = (b[j], b[j+1]) over [numa, n-j]
Lmo_row:
    ldp     x4, x5, [x23], #16       // v0, v1
    mov     x12, xzr                 // cy0 = 0
    mov     x13, xzr                 // cy1 = 0
    mov     x2, x22                  // len = n - j
    tbz     x22, #0, Lmo_p1
    ldr     x6, [x1], #8
    mul     x8, x6, x4               // lo0
    umulh   x9, x6, x4               // hi0
    mul     x10, x6, x5              // lo1
    umulh   x11, x6, x5              // hi1
    ldr     x14, [x0]
    adds    x8, x8, x14              // t = lo0 + rp     c1
    adcs    x9, x9, x10              // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr            // h2 = hi1 + c2
    adds    x8, x8, x12              // s = t + cy0      c4
    str     x8, [x0], #8
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1

Lmo_p1:
    tbz     x22, #1, Lmo_p2
    ldp     x6, x7, [x1], #16        // u_0, u_1
    ldr     x14, [x0]
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x8, x8, x14              // t
    adcs    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s
    str     x8, [x0], #8
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    ldr     x14, [x0]
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x8, x8, x14              // t
    adcs    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s
    str     x8, [x0], #8
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1

Lmo_p2:
    lsr     x2, x2, #2
    cbz     x2, Lmo_pend             // tail stores dropped
.p2align 4

Lmo_ptop:
    ldp     x6, x7, [x1], #32        // u_0, u_1
    ldp     x16, x17, [x1, #-16]     // u_2, u_3
    ldp     x14, x15, [x0]           // rp_0, rp_1
    mul     x8, x6, x4               // lo0
    umulh   x9, x6, x4               // hi0
    mul     x10, x6, x5              // lo1
    umulh   x11, x6, x5              // hi1
    adds    x8, x8, x14              // t = lo0 + rp     c1
    adcs    x9, x9, x10              // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr            // h2 = hi1 + c2
    adds    x8, x8, x12              // s = t + cy0      c4
    adcs    x12, x9, x13             // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr            // cy1 = h2 + c5
    mul     x9, x7, x4               // lo0'
    umulh   x10, x7, x4              // hi0'
    mul     x11, x7, x5              // lo1'
    umulh   x14, x7, x5              // hi1'
    adds    x9, x9, x15              // t'
    adcs    x10, x10, x11            // h'
    adcs    x14, x14, xzr            // h2'
    adds    x9, x9, x12              // s'
    adcs    x12, x10, x13            // cy0
    adcs    x13, x14, xzr            // cy1
    stp     x8, x9, [x0], #16
    ldp     x14, x15, [x0]           // rp_2, rp_3
    mul     x8, x16, x4              // lo0
    umulh   x9, x16, x4              // hi0
    mul     x10, x16, x5             // lo1
    umulh   x11, x16, x5             // hi1
    adds    x8, x8, x14              // t
    adcs    x9, x9, x10              // h
    adcs    x11, x11, xzr            // h2
    adds    x8, x8, x12              // s
    adcs    x12, x9, x13             // cy0
    adcs    x13, x11, xzr            // cy1
    mul     x9, x17, x4              // lo0'
    umulh   x10, x17, x4             // hi0'
    mul     x11, x17, x5             // lo1'
    umulh   x14, x17, x5             // hi1'
    adds    x9, x9, x15              // t'
    adcs    x10, x10, x11            // h'
    adcs    x14, x14, xzr            // h2'
    adds    x9, x9, x12              // s'
    adcs    x12, x10, x13            // cy0
    adcs    x13, x14, xzr            // cy1
    stp     x8, x9, [x0], #16
    subs    x2, x2, #1
    b.ne    Lmo_ptop

Lmo_pend:                            // tail stores dropped (truncation)
    sub     x0, x0, x22, lsl #3      // rp += 16 (next row)
    add     x0, x0, #16
    sub     x1, x1, x22, lsl #3      // up = numa
    subs    x22, x22, #2
    cmp     x22, #2
    b.ge    Lmo_row

Lmo_left:                            // n odd: dst[n-1] += lo(a[0]*b[n-1])
    tbnz    x21, #0, Lmo_odd
    b       Lmo_done                 // n == 2: seed row was everything

Lmo_odd:
    sub     x8, x21, #1
    ldr     x9, [x3, x8, lsl #3]     // b[n-1]
    ldr     x10, [x20]
    mul     x11, x10, x9
    ldr     x12, [x19, x8, lsl #3]
    add     x12, x12, x11
    str     x12, [x19, x8, lsl #3]   // carry dropped

Lmo_done:
    ldp     x23, x24, [sp, #48]      // restore regs, ret
    ldp     x21, x22, [sp, #32]
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #64
    ret

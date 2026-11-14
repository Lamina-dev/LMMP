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
// void lmmp_mul_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);
//
//   dst=x0, numa=x1, na=x2, numb=x3, nb=x4

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_mul_basecase_
// ======================================
// x19=dst, x20=numa, x21=na, x22=numb, x23=nb, x24=j,
// x25=numb walker (pairs), x26=dst walker (pairs)
// kernel temps: x4=v0, x5=v1, x12=cy0, x13=cy1, x6/x7/x16/x17=u,
// x8/x9/x10/x11/x14=prod+sums, x0/x1/x2=rp/up/n walkers
.globl ASM_GSYM(lmmp_mul_basecase_)
.p2align 2
ASM_GSYM(lmmp_mul_basecase_):
    stp     x29, x30, [sp, #-96]!       // save regs
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    stp     x25, x26, [sp, #64]
    mov     x19, x0                     // dst
    mov     x20, x1                     // numa
    mov     x21, x2                     // na
    mov     x22, x3                     // numb
    mov     x23, x4                     // nb

    cmp     x23, #1
    b.ne    Lmb_seed

    // nb == 1: [dst, na+1] = [numa, na] * numb[0]
    ldr     x3, [x22]
    mov     x0, x19
    mov     x1, x20
    mov     x2, x21
    bl      ASM_GSYM(lmmp_mul_1_)
    str     x0, [x19, x21, lsl #3]      // dst[na] = carry
    b       Lmb_done

    // seed mul_2: [dst, na+2] = [numa, na] * (numb[0], numb[1]); rp load-free
Lmb_seed:
    ldp     x4, x5, [x22]               // v0, v1
    mov     x12, xzr                    // cy0 = 0
    mov     x13, xzr                    // cy1 = 0
    mov     x0, x19
    mov     x1, x20
    mov     x2, x21
    tbz     x2, #0, Ls_1
    ldr     x6, [x1], #8                // u0
    mul     x8, x6, x4                  // lo0
    umulh   x9, x6, x4                  // hi0
    mul     x10, x6, x5                 // lo1
    umulh   x11, x6, x5                 // hi1
    adds    x9, x9, x10                 // h = hi0 + lo1   (c1 = 0)
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = lo0 + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13                // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr               // cy1 = h2 + c5

Ls_1:
    tbz     x2, #1, Ls_2
    ldp     x6, x7, [x1], #16           // u0, u1
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x9, x9, x10                 // h
    adcs    x11, x11, xzr               // h2
    adds    x8, x8, x12                 // s0 = lo0 + cy0
    adcs    x12, x9, x13                // cy0
    adcs    x13, x11, xzr               // cy1
    str     x8, [x0], #8
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x9, x9, x10                 // h
    adcs    x11, x11, xzr               // h2
    adds    x8, x8, x12                 // s1 = lo0 + cy0
    adcs    x12, x9, x13                // cy0
    adcs    x13, x11, xzr               // cy1
    str     x8, [x0], #8

Ls_2:
    lsr     x2, x2, #2
    cbz     x2, Ls_end
.p2align 4

Ls_top:
    ldp     x6, x7, [x1], #32           // u0, u1
    ldp     x16, x17, [x1, #-16]        // u2, u3
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x9, x9, x10                 // h
    adcs    x11, x11, xzr               // h2
    adds    x8, x8, x12                 // s0 = lo0 + cy0
    adcs    x12, x9, x13                // cy0
    adcs    x13, x11, xzr               // cy1
    mul     x9, x7, x4
    umulh   x10, x7, x4
    mul     x11, x7, x5
    umulh   x14, x7, x5
    adds    x10, x10, x11               // h'
    adcs    x14, x14, xzr               // h2'
    adds    x9, x9, x12                 // s1
    adcs    x12, x10, x13               // cy0
    adcs    x13, x14, xzr               // cy1
    stp     x8, x9, [x0], #16
    mul     x8, x16, x4
    umulh   x9, x16, x4
    mul     x10, x16, x5
    umulh   x11, x16, x5
    adds    x9, x9, x10                 // h
    adcs    x11, x11, xzr               // h2
    adds    x8, x8, x12                 // s2
    adcs    x12, x9, x13                // cy0
    adcs    x13, x11, xzr               // cy1
    mul     x9, x17, x4
    umulh   x10, x17, x4
    mul     x11, x17, x5
    umulh   x14, x17, x5
    adds    x10, x10, x11               // h'
    adcs    x14, x14, xzr               // h2'
    adds    x9, x9, x12                 // s3
    adcs    x12, x10, x13               // cy0
    adcs    x13, x14, xzr               // cy1
    stp     x8, x9, [x0], #16
    subs    x2, x2, #1
    b.ne    Ls_top

Ls_end:
    str     x12, [x0]                   // col na = cy0
    str     x13, [x0, #8]               // col na+1 = cy1

    // pair rows: j = 2, 4, ... while j+2 <= nb
    mov     x24, #2
    add     x3, x24, #2
    cmp     x3, x23
    b.hi    Lmb_odd_check
    add     x25, x22, #16               // numb walker at j
    add     x26, x19, #16               // dst walker at j
.p2align 4

Lmb_pair:                               // [dst+j, na+2] += [numa, na]*(numb[j],numb[j+1])
    ldp     x4, x5, [x25], #16          // v0, v1
    mov     x0, x26                     // rp = dst + j
    mov     x1, x20                     // up = numa
    mov     x2, x21                     // n = na
    mov     x12, xzr                    // cy0 = 0
    mov     x13, xzr                    // cy1 = 0
    tbz     x2, #0, Lp_1
    ldr     x6, [x1], #8
    mul     x8, x6, x4                  // lo0
    umulh   x9, x6, x4                  // hi0
    mul     x10, x6, x5                 // lo1
    umulh   x11, x6, x5                 // hi1
    ldr     x14, [x0]
    adds    x8, x8, x14                 // t = lo0 + rp
    adcs    x9, x9, x10                 // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13                // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr               // cy1 = h2 + c5

Lp_1:
    tbz     x2, #1, Lp_2
    ldp     x6, x7, [x1], #16           // u0, u1
    ldr     x14, [x0]
    mul     x8, x6, x4
    umulh   x9, x6, x4
    mul     x10, x6, x5
    umulh   x11, x6, x5
    adds    x8, x8, x14                 // t = lo0 + rp
    adcs    x9, x9, x10                 // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = t + cy0
    str     x8, [x0], #8
    adcs    x12, x9, x13                // cy0
    adcs    x13, x11, xzr               // cy1
    ldr     x14, [x0]
    mul     x8, x7, x4
    umulh   x9, x7, x4
    mul     x10, x7, x5
    umulh   x11, x7, x5
    adds    x8, x8, x14                 // t
    adcs    x9, x9, x10                 // h
    adcs    x11, x11, xzr               // h2
    adds    x8, x8, x12                 // s
    str     x8, [x0], #8
    adcs    x12, x9, x13                // cy0
    adcs    x13, x11, xzr               // cy1

Lp_2:
    lsr     x2, x2, #2
    cbz     x2, Lp_end
.p2align 4

Lp_top:
    ldp     x6, x7, [x1], #32           // u0, u1
    ldp     x16, x17, [x1, #-16]        // u2, u3
    ldp     x14, x15, [x0]              // rp0, rp1
    mul     x8, x6, x4                  // lo0
    umulh   x9, x6, x4                  // hi0
    mul     x10, x6, x5                 // lo1
    umulh   x11, x6, x5                 // hi1
    adds    x8, x8, x14                 // t = lo0 + rp
    adcs    x9, x9, x10                 // h = hi0 + lo1 + c1
    adcs    x11, x11, xzr               // h2 = hi1 + c2
    adds    x8, x8, x12                 // s = t + cy0
    adcs    x12, x9, x13                // cy0 = h + cy1 + c4
    adcs    x13, x11, xzr               // cy1 = h2 + c5
    mul     x9, x7, x4                  // lo0'
    umulh   x10, x7, x4                 // hi0'
    mul     x11, x7, x5                 // lo1'
    umulh   x14, x7, x5                 // hi1'
    adds    x9, x9, x15                 // t' = lo0' + rp
    adcs    x10, x10, x11               // h' = hi0' + lo1' + c1
    adcs    x14, x14, xzr               // h2' = hi1' + c2
    adds    x9, x9, x12                 // s' = t' + cy0
    adcs    x12, x10, x13               // cy0 = h' + cy1 + c4
    adcs    x13, x14, xzr               // cy1 = h2' + c5
    stp     x8, x9, [x0], #16
    ldp     x14, x15, [x0]              // rp2, rp3
    mul     x8, x16, x4                 // lo0
    umulh   x9, x16, x4                 // hi0
    mul     x10, x16, x5                // lo1
    umulh   x11, x16, x5                // hi1
    adds    x8, x8, x14                 // t
    adcs    x9, x9, x10                 // h
    adcs    x11, x11, xzr               // h2
    adds    x8, x8, x12                 // s
    adcs    x12, x9, x13                // cy0
    adcs    x13, x11, xzr               // cy1
    mul     x9, x17, x4                 // lo0'
    umulh   x10, x17, x4                // hi0'
    mul     x11, x17, x5                // lo1'
    umulh   x14, x17, x5                // hi1'
    adds    x9, x9, x15                 // t'
    adcs    x10, x10, x11               // h'
    adcs    x14, x14, xzr               // h2'
    adds    x9, x9, x12                 // s'
    adcs    x12, x10, x13               // cy0
    adcs    x13, x14, xzr               // cy1
    stp     x8, x9, [x0], #16
    subs    x2, x2, #1
    b.ne    Lp_top

Lp_end:
    str     x12, [x0]                   // col j+na = cy0
    str     x13, [x0, #8]               // col j+na+1 = cy1
    add     x26, x26, #16
    add     x24, x24, #2                // j += 2
    add     x3, x24, #2
    cmp     x3, x23
    b.ls    Lmb_pair                    // j+2 <= nb

Lmb_odd_check:
    cmp     x24, x23
    b.hs    Lmb_done                    // nb even: done
    // odd leftover row: [dst+j, na+1] += [numa, na] * numb[j]
    ldr     x3, [x22, x24, lsl #3]      // v = numb[j]
    add     x0, x19, x24, lsl #3        // rp = dst + j
    mov     x1, x20
    mov     x2, x21
    adds    x15, xzr, xzr               // carry = 0, C = 0
    tbz     x2, #0, Lmb_am_bx0          // n mod 2 == 0
    ldr     x4, [x1], #8                // up[0]
    mul     x8, x4, x3                  // lo0
    umulh   x12, x4, x3                 // hi0
    ldr     x4, [x0]
    adds    x8, x4, x8                  // s0 = rp[0] + lo0
    cinc    x15, x12, cs                // carry = hi0 + C
    str     x8, [x0], #8

Lmb_am_bx0:                             // first two limbs
    tbz     x2, #1, Lmb_am_2
    ldp     x4, x5, [x1], #16           // up[0], up[1]
    mul     x8, x4, x3                  // lo0
    umulh   x12, x4, x3                 // hi0
    mul     x9, x5, x3                  // lo1
    umulh   x13, x5, x3                 // hi1
    adds    x8, x8, x15                 // s0 = lo0 + carry
    adcs    x9, x9, x12                 // s1 = lo1 + hi0 + C
    ldp     x4, x5, [x0]                // rp[0], rp[1]
    adc     x15, x13, xzr               // carry = hi1 + C
    adds    x8, x4, x8                  // s0 += rp[0]
    adcs    x9, x5, x9                  // s1 += rp[1]
    cinc    x15, x15, cs                // carry += C
    stp     x8, x9, [x0], #16

Lmb_am_2:
    lsr     x2, x2, #2                  // remain 4-limbs size
    cbz     x2, Lmb_am_le3              // size == 0
    ldp     x4, x5, [x1], #32           // first a0, a1
    ldp     x6, x7, [x1, #-16]          // first a2, a3
    b       Lmb_am_mid

Lmb_am_le3:
    mov     x0, x15                     // carry
    b       Lmb_am_store
.p2align 4

Lmb_am_top:                             // for each 4 limb: s_j += rp[j]
    ldp     x4, x5, [x1], #32
    ldp     x6, x7, [x1, #-16]
    adds    x8, x16, x8                 // s0 += rp[0]
    adcs    x9, x17, x9                 // s1 += rp[1]
    stp     x8, x9, [x0], #32
    adcs    x10, x12, x10               // s2 += rp[2]
    adcs    x11, x13, x11               // s3 += rp[3]
    stp     x10, x11, [x0, #-16]
    cinc    x15, x15, cs                // carry += C

Lmb_am_mid:                             // products, carry chain
    sub     x2, x2, #1
    mul     x8, x4, x3                  // lo0
    umulh   x12, x4, x3                 // hi0
    mul     x9, x5, x3                  // lo1
    umulh   x13, x5, x3                 // hi1
    adds    x8, x8, x15                 // s0 = lo0 + carry
    mul     x10, x6, x3                 // lo2
    umulh   x14, x6, x3                 // hi2
    adcs    x9, x9, x12                 // s1 = lo1 + hi0 + C
    mul     x11, x7, x3                 // lo3
    umulh   x15, x7, x3                 // hi3
    adcs    x10, x10, x13               // s2 = lo2 + hi1 + C
    ldp     x16, x17, [x0]              // rp[0], rp[1]
    adcs    x11, x11, x14               // s3 = lo3 + hi2 + C
    ldp     x12, x13, [x0, #16]         // rp[2], rp[3]
    adc     x15, x15, xzr               // carry = hi3 + C
    cbnz    x2, Lmb_am_top
    adds    x8, x16, x8                 // last: s0 += rp[0]
    adcs    x9, x17, x9                 // s1 += rp[1]
    adcs    x10, x12, x10               // s2 += rp[2]
    adcs    x11, x13, x11               // s3 += rp[3]
    stp     x8, x9, [x0]
    stp     x10, x11, [x0, #16]
    cinc    x0, x15, cs                 // x0 = carry + C

Lmb_am_store:
    add     x5, x21, x24                // dst[na + j] = carry
    str     x0, [x19, x5, lsl #3]

Lmb_done:
    ldp     x25, x26, [sp, #64]         // restore regs, ret
    ldp     x23, x24, [sp, #48]
    ldp     x21, x22, [sp, #32]
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #96
    ret

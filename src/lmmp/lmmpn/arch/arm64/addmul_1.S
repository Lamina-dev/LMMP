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
// mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);
//
//   rp=x0, up=x1, n=x2, v=x3

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_addmul_1_
// ======================================
.globl ASM_GSYM(lmmp_addmul_1_)
.p2align 2
ASM_GSYM(lmmp_addmul_1_):
    cmp     x2, #28
    b.hs    Lam1n_enter                  // n >= 28: fused form
    adds    x15, xzr, xzr                // carry = 0, C = 0
    tbz     x2, #0, Lam1_1               // n mod 2 == 0
    ldr     x4, [x1], #8                 // up[0]
    mul     x8, x4, x3                   // lo0
    umulh   x12, x4, x3                  // hi0
    ldr     x4, [x0]
    adds    x8, x4, x8                   // s0 = rp[0] + lo0
    cinc    x15, x12, cs                 // carry = hi0 + C
    str     x8, [x0], #8

Lam1_1:                                  // takle fisrt two limbs
    tbz     x2, #1, Lam1_2
    ldp     x4, x5, [x1], #16            // up[0], up[1]
    mul     x8, x4, x3                   // lo0
    umulh   x12, x4, x3                  // hi0
    mul     x9, x5, x3                   // lo1
    umulh   x13, x5, x3                  // hi1
    adds    x8, x8, x15                  // s0 = lo0 + carry
    adcs    x9, x9, x12                  // s1 = lo1 + hi0 + C
    ldp     x4, x5, [x0]                 // rp[0], rp[1]
    adc     x15, x13, xzr                // carry = hi1 + C
    adds    x8, x4, x8                   // s0 += rp[0]
    adcs    x9, x5, x9                   // s1 += rp[1]
    cinc    x15, x15, cs                 // carry += C
    stp     x8, x9, [x0], #16

Lam1_2:
    lsr     x2, x2, #2                   // remain 4-limbs size
    cbz     x2, Lam1_le3                 // size==0, retrun
    ldp     x4, x5, [x1], #32            // first a0, a1
    ldp     x6, x7, [x1, #-16]           // first a2, a3
    b       Lam1_mid

Lam1_le3:
    mov     x0, x15                      // ret carry
    ret
.p2align 4

Lam1_top:                                // for each 4 limb: s_j += rp[j]
    ldp     x4, x5, [x1], #32
    ldp     x6, x7, [x1, #-16]
    adds    x8, x16, x8                  // s0 += rp[0]
    adcs    x9, x17, x9                  // s1 += rp[1]
    stp     x8, x9, [x0], #32
    adcs    x10, x12, x10                // s2 += rp[2]
    adcs    x11, x13, x11                // s3 += rp[3]
    stp     x10, x11, [x0, #-16]
    cinc    x15, x15, cs                 // carry += C

Lam1_mid:
    sub     x2, x2, #1
    mul     x8, x4, x3                   // lo0
    umulh   x12, x4, x3                  // hi0
    mul     x9, x5, x3                   // lo1
    umulh   x13, x5, x3                  // hi1
    adds    x8, x8, x15                  // s0 = lo0 + carry
    mul     x10, x6, x3                  // lo2
    umulh   x14, x6, x3                  // hi2
    adcs    x9, x9, x12                  // s1 = lo1 + hi0 + C
    mul     x11, x7, x3                  // lo3
    umulh   x15, x7, x3                  // hi3
    adcs    x10, x10, x13                // s2 = lo2 + hi1 + C
    ldp     x16, x17, [x0]               // rp[0], rp[1]
    adcs    x11, x11, x14                // s3 = lo3 + hi2 + C
    ldp     x12, x13, [x0, #16]          // rp[2], rp[3]
    adc     x15, x15, xzr                // carry = hi3 + C
    cbnz    x2, Lam1_top
    adds    x8, x16, x8                  // last: s0 += rp[0]
    adcs    x9, x17, x9                  // s1 += rp[1]
    adcs    x10, x12, x10                // s2 += rp[2]
    adcs    x11, x13, x11                // s3 += rp[3]
    stp     x8, x9, [x0]
    stp     x10, x11, [x0, #16]
    cinc    x0, x15, cs                  // ret carry + C
    ret

// n >= 28: fused two-chain form
Lam1n_enter:
    mov     x17, #0                     // CY = 0
    lsr     x16, x2, #2                 // 4-limb block count
    tbz     x2, #0, Lam1n_bx0

Lam1n_bx1:                              // leading single limb
    ldr     x4, [x1], #8
    mul     x8, x4, x3                  // lo0
    umulh   x4, x4, x3                  // hi0
    tbz     x2, #1, Lam1n_b01

Lam1n_b11:                              // three leading limbs
    ldp     x5, x6, [x1], #16
    ldp     x12, x13, [x0]
    ldr     x14, [x0, #16]
    mul     x9, x5, x3                  // lo1
    umulh   x5, x5, x3                  // hi1
    mul     x10, x6, x3                 // lo2
    umulh   x6, x6, x3                  // hi2
    adds    x8, x12, x8                 // A: rp0 + lo0
    adcs    x4, x13, x4                 // A: rp1 + hi0
    adcs    x5, x14, x5                 // A: rp2 + hi1
    cinc    x6, x6, cs                  // A: hi2 + C
    adds    x8, x8, x17                 // B: s0 + CY
    adcs    x4, x4, x9                  // B: s1 + lo1
    adcs    x5, x5, x10                 // B: s2 + lo2
    cinc    x17, x6, cs                 // CY out
    stp     x8, x4, [x0], #16
    str     x5, [x0], #8
    cbnz    x16, Lam1n_top
    mov     x0, x17
    ret

Lam1n_b01:                              // single limb only
    ldr     x12, [x0]
    adds    x8, x12, x8                 // A: rp0 + lo0
    cinc    x4, x4, cs                  // A: hi0 + C
    adds    x8, x8, x17                 // B: s0 + CY
    cinc    x17, x4, cs                 // CY out
    str     x8, [x0], #8
    cbnz    x16, Lam1n_top
    mov     x0, x17
    ret

Lam1n_bx0:
    ldp     x4, x5, [x1], #16
    tbz     x2, #1, Lam1n_ent           // n%4==0 -> mid entry

Lam1n_b10:                              // two leading limbs
    ldp     x12, x13, [x0]
    mul     x8, x4, x3                  // lo0
    umulh   x4, x4, x3                  // hi0
    mul     x9, x5, x3                  // lo1
    umulh   x5, x5, x3                  // hi1
    adds    x8, x12, x8                 // A: rp0 + lo0
    adcs    x4, x13, x4                 // A: rp1 + hi0
    cinc    x5, x5, cs                  // A: hi1 + C
    adds    x8, x8, x17                 // B: s0 + CY
    adcs    x4, x4, x9                  // B: s1 + lo1
    cinc    x17, x5, cs                 // CY out
    stp     x8, x4, [x0], #16
    cbz     x16, Lam1n_done

.p2align 4
Lam1n_top:
    ldp     x4, x5, [x1], #16
Lam1n_ent:                              // n%4==0 entry, a0/a1 preloaded
    ldp     x6, x7, [x1], #16
    ldp     x12, x13, [x0]
    ldp     x14, x15, [x0, #16]
    mul     x8, x4, x3                  // lo0
    umulh   x4, x4, x3                  // hi0
    mul     x9, x5, x3                  // lo1
    umulh   x5, x5, x3                  // hi1
    mul     x10, x6, x3                 // lo2
    umulh   x6, x6, x3                  // hi2
    mul     x11, x7, x3                 // lo3
    umulh   x7, x7, x3                  // hi3
    adds    x8, x12, x8                 // A: rp0 + lo0
    adcs    x4, x13, x4                 // A: rp1 + hi0
    adcs    x5, x14, x5                 // A: rp2 + hi1
    adcs    x6, x15, x6                 // A: rp3 + hi2
    cinc    x7, x7, cs                  // A: hi3 + C
    adds    x8, x8, x17                 // B: s0 + CY
    adcs    x4, x4, x9                  // B: s1 + lo1
    adcs    x5, x5, x10                 // B: s2 + lo2
    adcs    x6, x6, x11                 // B: s3 + lo3
    cinc    x17, x7, cs                 // B: CY out
    stp     x8, x4, [x0], #16
    stp     x5, x6, [x0], #16
    sub     x16, x16, #1
    cbnz    x16, Lam1n_top

Lam1n_done:
    mov     x0, x17
    ret

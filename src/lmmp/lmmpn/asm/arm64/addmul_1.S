//  Copyright (C) 2026 HJimmyK(Jericho Knox)
//
//  This file is part of LMMP.
//
//  LMMP is free software: you can redistribute it and/or modify it under
//  the terms of the GNU Lesser General Public License (LGPL) as published
//  by the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed WITHOUT ANY WARRANTY.
//
//  See <https://www.gnu.org/licenses/>.

// mp_limb_t lmmp_addmul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);
//
//   x0=rp, x1=up, x2=n, x3=v
//   ret x0 = highest limb carry
//   cal: rp[i] += up[i]*v

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_addmul_1_)
.p2align 2

ASM_GSYM(lmmp_addmul_1_):
    adds    x15, xzr, xzr                // carry = 0, C = 0
    tbz     x2, #0, Lam1_1               // n mod 2 == 0
    ldr     x4, [x1], #8                 // up[0]
    mul     x8, x4, x3                   // lo_0
    umulh   x12, x4, x3                  // hi_0
    ldr     x4, [x0]
    adds    x8, x4, x8                   // s_0 = rp[0] + lo_0
    cinc    x15, x12, cs                 // carry = hi_0 + C
    str     x8, [x0], #8

Lam1_1:                                  // takle fisrt two limbs
    tbz     x2, #1, Lam1_2
    ldp     x4, x5, [x1], #16            // up[0], up[1]
    mul     x8, x4, x3                   // lo_0
    umulh   x12, x4, x3                  // hi_0
    mul     x9, x5, x3                   // lo_1
    umulh   x13, x5, x3                  // hi_1
    adds    x8, x8, x15                  // s_0 = lo_0 + carry
    adcs    x9, x9, x12                  // s_1 = lo_1 + hi_0 + C
    ldp     x4, x5, [x0]                 // rp[0], rp[1]
    adc     x15, x13, xzr                // carry = hi_1 + C
    adds    x8, x4, x8                   // s_0 += rp[0]
    adcs    x9, x5, x9                   // s_1 += rp[1]
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
    adds    x8, x16, x8                  // s_0 += rp[0]
    adcs    x9, x17, x9                  // s_1 += rp[1]
    stp     x8, x9, [x0], #32
    adcs    x10, x12, x10                // s_2 += rp[2]
    adcs    x11, x13, x11                // s_3 += rp[3]
    stp     x10, x11, [x0, #-16]
    cinc    x15, x15, cs                 // carry += C

Lam1_mid:
    sub     x2, x2, #1
    mul     x8, x4, x3                   // lo_0
    umulh   x12, x4, x3                  // hi_0
    mul     x9, x5, x3                   // lo_1
    umulh   x13, x5, x3                  // hi_1
    adds    x8, x8, x15                  // s_0 = lo_0 + carry
    mul     x10, x6, x3                  // lo_2
    umulh   x14, x6, x3                  // hi_2
    adcs    x9, x9, x12                  // s_1 = lo_1 + hi_0 + C
    mul     x11, x7, x3                  // lo_3
    umulh   x15, x7, x3                  // hi_3
    adcs    x10, x10, x13                // s_2 = lo_2 + hi_1 + C
    ldp     x16, x17, [x0]               // rp[0], rp[1]
    adcs    x11, x11, x14                // s_3 = lo_3 + hi_2 + C
    ldp     x12, x13, [x0, #16]          // rp[2], rp[3]
    adc     x15, x15, xzr                // carry = hi_3 + C
    cbnz    x2, Lam1_top
    adds    x8, x16, x8                  // last: s_0 += rp[0]
    adcs    x9, x17, x9                  // s_1 += rp[1]
    adcs    x10, x12, x10                // s_2 += rp[2]
    adcs    x11, x13, x11                // s_3 += rp[3]
    stp     x8, x9, [x0]
    stp     x10, x11, [x0, #16]
    cinc    x0, x15, cs                  // ret carry + C
    ret

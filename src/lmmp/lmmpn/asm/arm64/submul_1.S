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

// mp_limb_t lmmp_submul_1_(mp_ptr numa, mp_srcptr numb, mp_size_t n, mp_limb_t b);
//
//   x0=rp, x1=up, x2=n, x3=v
//   ret x0 = highest borrow
//   cal: rp[i] -= up[i]*v, s_j = lo_j + hi_{j-1} + C

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_submul_1_)
.p2align 2

ASM_GSYM(lmmp_submul_1_):
    adds    x15, xzr, xzr                // borrow = 0, C = 0
    tbz     x2, #0, Lsm1_1               // n mod 2 == 0
    ldr     x4, [x1], #8                 // up[0]
    mul     x8, x4, x3                   // lo_0
    umulh   x12, x4, x3                  // hi_0
    ldr     x4, [x0]
    subs    x8, x4, x8                   // rp[0] -= lo_0
    cinc    x15, x12, cc                 // borrow = hi_0 + borrow
    str     x8, [x0], #8

Lsm1_1:                                  // first two limbs
    tbz     x2, #1, Lsm1_2
    ldp     x4, x5, [x1], #16            // up[0], up[1]
    mul     x8, x4, x3                   // lo_0
    umulh   x12, x4, x3                  // hi_0
    mul     x9, x5, x3                   // lo_1
    umulh   x13, x5, x3                  // hi_1
    adds    x8, x8, x15                  // s_0 = lo_0 + borrow
    adcs    x9, x9, x12                  // s_1 = lo_1 + hi_0 + C
    ldp     x4, x5, [x0]                 // rp[0], rp[1]
    adc     x15, x13, xzr                // borrow = hi_1 + C
    subs    x8, x4, x8                   // rp[0] -= s_0
    sbcs    x9, x5, x9                   // rp[1] -= s_1
    cinc    x15, x15, cc                 // borrow += borrow
    stp     x8, x9, [x0], #16

Lsm1_2:
    lsr     x2, x2, #2                   // remain 4-limbs size
    cbz     x2, Lsm1_le3                 // size == 0, return
    ldp     x4, x5, [x1], #32            // first a0, a1
    ldp     x6, x7, [x1, #-16]           // first a2, a3
    b       Lsm1_mid

Lsm1_le3:
    mov     x0, x15                      // ret borrow
    ret
.p2align 4

Lsm1_top:                                // for each 4 limb: rp[j] -= s_j
    ldp     x4, x5, [x1], #32
    ldp     x6, x7, [x1, #-16]
    subs    x8, x16, x8                  // rp[0] -= s_0
    sbcs    x9, x17, x9                  // rp[1] -= s_1
    stp     x8, x9, [x0], #32
    sbcs    x10, x12, x10                // rp[2] -= s_2
    sbcs    x11, x13, x11                // rp[3] -= s_3
    stp     x10, x11, [x0, #-16]
    cinc    x15, x15, cc                 // borrow += borrow

Lsm1_mid:                                // products, carry chain
    sub     x2, x2, #1
    mul     x8, x4, x3                   // lo_0
    umulh   x12, x4, x3                  // hi_0
    mul     x9, x5, x3                   // lo_1
    umulh   x13, x5, x3                  // hi_1
    adds    x8, x8, x15                  // s_0 = lo_0 + borrow
    mul     x10, x6, x3                  // lo_2
    umulh   x14, x6, x3                  // hi_2
    adcs    x9, x9, x12                  // s_1 = lo_1 + hi_0 + C
    mul     x11, x7, x3                  // lo_3
    umulh   x15, x7, x3                  // hi_3
    adcs    x10, x10, x13                // s_2 = lo_2 + hi_1 + C
    ldp     x16, x17, [x0]               // rp[0], rp[1]
    adcs    x11, x11, x14                // s_3 = lo_3 + hi_2 + C
    ldp     x12, x13, [x0, #16]          // rp[2], rp[3]
    adc     x15, x15, xzr                // borrow = hi_3 + C
    cbnz    x2, Lsm1_top
    subs    x8, x16, x8                  // last: rp[0] -= s_0
    sbcs    x9, x17, x9                  // rp[1] -= s_1
    sbcs    x10, x12, x10                // rp[2] -= s_2
    sbcs    x11, x13, x11                // rp[3] -= s_3
    stp     x8, x9, [x0]
    stp     x10, x11, [x0, #16]
    cinc    x0, x15, cc                  // ret borrow + borrow
    ret

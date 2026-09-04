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

// void lmmp_mul_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);
//
//   x0=dst, x1=numa, x2=na, x3=numb, x4=nb
//   ret: none
//   cal: dst[0..na+nb) = numa * numb, mul_1 then addmul_1 loop

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mul_basecase_)
.p2align 2

ASM_GSYM(lmmp_mul_basecase_):
    stp     x29, x30, [sp, #-80]!       // save regs
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    mov     x19, x0                     // x19 = dst
    mov     x20, x1                     // x20 = numa
    mov     x21, x2                     // x21 = na
    mov     x22, x3                     // x22 = numb
    mov     x23, x4                     // x23 = nb
    ldr     x24, [x22]                  // x24 = numb[0]
    mov     x0, x19
    mov     x1, x20
    mov     x2, x21
    mov     x3, x24
    bl      ASM_GSYM(lmmp_mul_1_)       // dst[0..na) = numa*numb[0]
    str     x0, [x19, x21, lsl #3]      // dst[na] = carry
    mov     x24, #1                     // j = 1
    cmp     x24, x23
    b.hs    Lmb_done                    // nb <= 1, done
.p2align 4

Lmb_outer:                              // dst[j..j+na] += numa * numb[j]
    ldr     x3, [x22, x24, lsl #3]      // v = numb[j]
    add     x0, x19, x24, lsl #3        // rp = dst + j
    mov     x1, x20                     // up = numa
    mov     x2, x21                     // n = na
    adds    x15, xzr, xzr               // carry = 0, C = 0
    tbz     x2, #0, Lmb_am_bx0          // n mod 2 == 0
    ldr     x4, [x1], #8                // up[0]
    mul     x8, x4, x3                  // lo_0
    umulh   x12, x4, x3                 // hi_0
    ldr     x4, [x0]
    adds    x8, x4, x8                  // s_0 = rp[0] + lo_0
    cinc    x15, x12, cs                // carry = hi_0 + C
    str     x8, [x0], #8

Lmb_am_bx0:                             // first two limbs
    tbz     x2, #1, Lmb_am_2
    ldp     x4, x5, [x1], #16           // up[0], up[1]
    mul     x8, x4, x3                  // lo_0
    umulh   x12, x4, x3                 // hi_0
    mul     x9, x5, x3                  // lo_1
    umulh   x13, x5, x3                 // hi_1
    adds    x8, x8, x15                 // s_0 = lo_0 + carry
    adcs    x9, x9, x12                 // s_1 = lo_1 + hi_0 + C
    ldp     x4, x5, [x0]                // rp[0], rp[1]
    adc     x15, x13, xzr               // carry = hi_1 + C
    adds    x8, x4, x8                  // s_0 += rp[0]
    adcs    x9, x5, x9                  // s_1 += rp[1]
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
    b       Lmb_am_end
.p2align 4

Lmb_am_top:                             // for each 4 limb: s_j += rp[j]
    ldp     x4, x5, [x1], #32
    ldp     x6, x7, [x1, #-16]
    adds    x8, x16, x8                 // s_0 += rp[0]
    adcs    x9, x17, x9                 // s_1 += rp[1]
    stp     x8, x9, [x0], #32
    adcs    x10, x12, x10               // s_2 += rp[2]
    adcs    x11, x13, x11               // s_3 += rp[3]
    stp     x10, x11, [x0, #-16]
    cinc    x15, x15, cs                // carry += C

Lmb_am_mid:                             // products, carry chain
    sub     x2, x2, #1
    mul     x8, x4, x3                  // lo_0
    umulh   x12, x4, x3                 // hi_0
    mul     x9, x5, x3                  // lo_1
    umulh   x13, x5, x3                 // hi_1
    adds    x8, x8, x15                 // s_0 = lo_0 + carry
    mul     x10, x6, x3                 // lo_2
    umulh   x14, x6, x3                 // hi_2
    adcs    x9, x9, x12                 // s_1 = lo_1 + hi_0 + C
    mul     x11, x7, x3                 // lo_3
    umulh   x15, x7, x3                 // hi_3
    adcs    x10, x10, x13               // s_2 = lo_2 + hi_1 + C
    ldp     x16, x17, [x0]              // rp[0], rp[1]
    adcs    x11, x11, x14               // s_3 = lo_3 + hi_2 + C
    ldp     x12, x13, [x0, #16]         // rp[2], rp[3]
    adc     x15, x15, xzr               // carry = hi_3 + C
    cbnz    x2, Lmb_am_top
    adds    x8, x16, x8                 // last: s_0 += rp[0]
    adcs    x9, x17, x9                 // s_1 += rp[1]
    adcs    x10, x12, x10               // s_2 += rp[2]
    adcs    x11, x13, x11               // s_3 += rp[3]
    stp     x8, x9, [x0]
    stp     x10, x11, [x0, #16]
    cinc    x0, x15, cs                 // x0 = carry + C

Lmb_am_end:
    add     x5, x21, x24                // dst[na + j] = carry
    str     x0, [x19, x5, lsl #3]
    add     x24, x24, #1                // j++
    cmp     x24, x23
    b.lo    Lmb_outer

Lmb_done:
    ldp     x19, x20, [sp, #16]         // restore regs, ret
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

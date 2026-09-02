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

// void lmmp_mullo_basecase_(mp_ptr dst, mp_srcptr numa, mp_srcptr numb, mp_size_t n);
//
//   x0=dst, x1=numa, x2=numb, x3=n
//   ret: none
//   cal: dst[0..n) = lo(numa * numb), addmul_1 columns

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mullo_basecase_)
.p2align 2

ASM_GSYM(lmmp_mullo_basecase_):
    stp     x26, x25, [sp, #-80]!       // save regs
    stp     x24, x23, [sp, #16]
    stp     x22, x21, [sp, #32]
    stp     x20, x19, [sp, #48]
    stp     x29, x30, [sp, #64]
    add     x29, sp, #64
    mov     x22, x2                     // x22 = numb
    mov     x19, x0                     // x19 = dst
    ldr     x8, [x1]                    // numa[0]
    subs    x2, x3, #1                  // n - 1
    ldr     x9, [x22, x2, lsl #3]       // numb[n-1]
    mul     x24, x9, x8                 // x24 = lo(numa[0]*numb[n-1])
    b.eq    Lmo_4                       // n == 1
    mov     x21, x3                     // x21 = n
    mov     x20, x1                     // x20 = numa
    ldr     x23, [x22]                  // numb[0]
    ldr     x25, [x1, x2, lsl #3]       // numa[n-1]
    mov     x0, x19
    mov     x1, x20
    mov     x2, x21
    sub     x2, x2, #1
    mov     x3, x23
    bl      ASM_GSYM(lmmp_mul_1_)       // dst[0..n-1) = numa[0..n-1)*numb[0]
    add     x8, x0, x24                 // mul_1 carry + x24
    madd    x24, x25, x23, x8           // x24 += numa[n-1]*numb[0]
    add     x19, x19, #8                // rp = dst + 1
    subs    x21, x21, #2                // remain = n - 2
    b.eq    Lmo_4                       // n == 2
    add     x23, x22, #8                // &numb[1]
.p2align 4

Lmo_3:                                  // dst[j..] += numa[0..len)*numb[j]
    ldr     x22, [x23], #8              // v = numb[j]
    ldr     x25, [x20, x21, lsl #3]     // numa[len]
    mov     x0, x19                     // rp = dst + j
    mov     x1, x20                     // up = numa
    mov     x2, x21                     // len
    mov     x3, x22                     // v
    adds    x15, xzr, xzr               // carry = 0, C = 0
    tbz     x2, #0, Lmo_am_bx0          // len mod 2 == 0
    ldr     x4, [x1], #8                // up[0]
    mul     x8, x4, x3                  // lo_0
    umulh   x12, x4, x3                 // hi_0
    ldr     x4, [x0]
    adds    x8, x4, x8                  // s_0 = rp[0] + lo_0
    cinc    x15, x12, cs                // carry = hi_0 + C
    str     x8, [x0], #8

Lmo_am_bx0:                             // first two limbs
    tbz     x2, #1, Lmo_am_2
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

Lmo_am_2:
    lsr     x2, x2, #2                  // remain 4-limbs size
    cbz     x2, Lmo_am_le3              // size == 0
    ldp     x4, x5, [x1], #32           // first a0, a1
    ldp     x6, x7, [x1, #-16]          // first a2, a3
    b       Lmo_am_mid

Lmo_am_le3:
    mov     x0, x15                     // carry
    b       Lmo_am_end
.p2align 4

Lmo_am_top:                             // for each 4 limb: s_j += rp[j]
    ldp     x4, x5, [x1], #32
    ldp     x6, x7, [x1, #-16]
    adds    x8, x16, x8                 // s_0 += rp[0]
    adcs    x9, x17, x9                 // s_1 += rp[1]
    stp     x8, x9, [x0], #32
    adcs    x10, x12, x10               // s_2 += rp[2]
    adcs    x11, x13, x11               // s_3 += rp[3]
    stp     x10, x11, [x0, #-16]
    cinc    x15, x15, cs                // carry += C

Lmo_am_mid:                             // products, carry chain
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
    cbnz    x2, Lmo_am_top
    adds    x8, x16, x8                 // last: s_0 += rp[0]
    adcs    x9, x17, x9                 // s_1 += rp[1]
    adcs    x10, x12, x10               // s_2 += rp[2]
    adcs    x11, x13, x11               // s_3 += rp[3]
    stp     x8, x9, [x0]
    stp     x10, x11, [x0, #16]
    cinc    x0, x15, cs                 // x0 = carry + C

Lmo_am_end:                             // x24 = numa[len]*v + carry + x24
    add     x8, x0, x24
    madd    x24, x25, x22, x8
    add     x19, x19, #8                // next column
    subs    x21, x21, #1                // remain -= 1
    b.ne    Lmo_3

Lmo_4:                                  // dst[n-1] = x24
    str     x24, [x19]
    ldp     x29, x30, [sp, #64]         // restore regs, ret
    ldp     x20, x19, [sp, #48]
    ldp     x22, x21, [sp, #32]
    ldp     x24, x23, [sp, #16]
    ldp     x26, x25, [sp], #80
    ret

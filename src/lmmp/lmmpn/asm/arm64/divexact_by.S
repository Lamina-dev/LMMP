//
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
//

//
// void lmmp_divexact_by3_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
// void lmmp_divexact_by9_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
// void lmmp_divexact_by15_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

#include "lmmp_asm.h"

.text

// ======================================
// lmmp_divexact_by3_
// ======================================
.p2align 2
.globl ASM_GSYM(lmmp_divexact_by3_)
ASM_GSYM(lmmp_divexact_by3_):
    movz    x8, #0xAAAB, lsl #48
    movk    x8, #0xAAAA, lsl #32        // INV3 = 3^(-1) mod B
    movk    x8, #0xAAAA, lsl #16
    movk    x8, #0xAAAA
    mov     x6, #3
    b       Ldivex_common

// ======================================
// lmmp_divexact_by9_
// ======================================
.p2align 2
.globl ASM_GSYM(lmmp_divexact_by9_)
ASM_GSYM(lmmp_divexact_by9_):
    movz    x8, #0x8E39, lsl #48
    movk    x8, #0x8E38, lsl #32        // INV9 = 9^(-1) mod B
    movk    x8, #0x8E38, lsl #16
    movk    x8, #0x8E38
    mov     x6, #9
    b       Ldivex_common

// ======================================
// lmmp_divexact_by15_
// ======================================
.p2align 2
.globl ASM_GSYM(lmmp_divexact_by15_)
ASM_GSYM(lmmp_divexact_by15_):
    movz    x8, #0xEEEF, lsl #48
    movk    x8, #0xEEEE, lsl #32        // INV15 = 15^(-1) mod B
    movk    x8, #0xEEEE, lsl #16
    movk    x8, #0xEEEE
    mov     x6, #15
    // fall through

// ======================================
// 共享核心: [dst,na] = [numa,na] / k
// ======================================
.p2align 2
Ldivex_common:
    mov     x7, #0                      // c = 0

Ldivex_loop:
    ldr     x9, [x1], #8                // s
    subs    x9, x9, x7                  // l = s - c, C = !borrow
    cset    x7, cc                      // c = borrow
    mul     x9, x9, x8                  // q = l * INV
    str     x9, [x0], #8
    umulh   x10, x9, x6                 // high(k*q)
    add     x7, x7, x10                 // c += high(k*q)
    subs    x2, x2, #1
    b.ne    Ldivex_loop
    ret

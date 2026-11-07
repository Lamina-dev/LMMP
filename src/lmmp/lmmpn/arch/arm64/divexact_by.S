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
// void lmmp_divexact_by3_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
// void lmmp_divexact_by9_(mp_ptr dst, mp_srcptr numa, mp_size_t na);
// void lmmp_divexact_by15_(mp_ptr dst, mp_srcptr numa, mp_size_t na);

//
//   dst=x0, numa=x1, na=x2
#include "lmmp_asm.h"

.text

// ======================================
// lmmp_divexact_by3_
// ======================================
.globl ASM_GSYM(lmmp_divexact_by3_)
.p2align 2
ASM_GSYM(lmmp_divexact_by3_):
    movz    x8, #0xAAAA, lsl #48
    movk    x8, #0xAAAA, lsl #32        // INV3 = 3^(-1) mod B
    movk    x8, #0xAAAA, lsl #16
    movk    x8, #0xAAAB
    mov     x10, #0                     // c = 0
    ands    x17, x2, #3                 // remainder limbs first
    b.eq    Lby3_groups

Lby3_scalar:
    ldr     x9, [x1], #8                // s
    subs    x12, x9, x10                // l = s - c, C = !borrow
    mul     x14, x12, x8                // q = l * INV3
    str     x14, [x0], #8
    lsr     x15, x14, #63               // u = [q >= 2^63]
    cinc    x15, x15, lo                // u + borrow
    cmp     x12, x14                    // lo = [l < q]
    cinc    x10, x15, lo                // c' = borrow + u + [l < q]
    subs    x17, x17, #1
    b.ne    Lby3_scalar

Lby3_groups:
    lsr     x2, x2, #2                  // 4 limbs per group
    cbz     x2, Lby3_ret

.p2align 2
Lby3_loop:
    ldp     x3, x4, [x1]                // s0, s1
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0]
    lsr     x15, x14, #63
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #8]
    lsr     x15, x14, #63
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    ldp     x3, x4, [x1, #16]           // s2, s3
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0, #16]
    lsr     x15, x14, #63
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #24]
    lsr     x15, x14, #63
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    add     x1, x1, #32
    add     x0, x0, #32
    subs    x2, x2, #1
    b.ne    Lby3_loop

Lby3_ret:
    ret

// ======================================
// lmmp_divexact_by9_
// ======================================
.globl ASM_GSYM(lmmp_divexact_by9_)
.p2align 2
ASM_GSYM(lmmp_divexact_by9_):
    movz    x8, #0x8E38, lsl #48
    movk    x8, #0xE38E, lsl #32        // INV9 = 9^(-1) mod B
    movk    x8, #0x38E3, lsl #16
    movk    x8, #0x8E39
    mov     x10, #0
    ands    x17, x2, #3
    b.eq    Lby9_groups

Lby9_scalar:
    ldr     x9, [x1], #8
    subs    x12, x9, x10
    mul     x14, x12, x8
    str     x14, [x0], #8
    lsr     x15, x14, #61               // hi = q >> 61
    cinc    x15, x15, lo                // hi + borrow
    cmp     x12, x14                    // lo = [l < q]
    cinc    x10, x15, lo
    subs    x17, x17, #1
    b.ne    Lby9_scalar

Lby9_groups:
    lsr     x2, x2, #2
    cbz     x2, Lby9_ret

.p2align 2
Lby9_loop:
    ldp     x3, x4, [x1]
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0]
    lsr     x15, x14, #61
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #8]
    lsr     x15, x14, #61
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    ldp     x3, x4, [x1, #16]
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0, #16]
    lsr     x15, x14, #61
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #24]
    lsr     x15, x14, #61
    cinc    x15, x15, lo
    cmp     x12, x14
    cinc    x10, x15, lo

    add     x1, x1, #32
    add     x0, x0, #32
    subs    x2, x2, #1
    b.ne    Lby9_loop

Lby9_ret:
    ret

// ======================================
// lmmp_divexact_by15_
// ======================================
.globl ASM_GSYM(lmmp_divexact_by15_)
.p2align 2
ASM_GSYM(lmmp_divexact_by15_):
    movz    x8, #0xEEEE, lsl #48
    movk    x8, #0xEEEE, lsl #32        // INV15 = 15^(-1) mod B
    movk    x8, #0xEEEE, lsl #16
    movk    x8, #0xEEEF
    mov     x10, #0
    ands    x17, x2, #3
    b.eq    Lby15_groups

Lby15_scalar:
    ldr     x9, [x1], #8
    subs    x12, x9, x10                // l = s - c, C = !borrow
    mul     x14, x12, x8                // q = l * INV15
    str     x14, [x0], #8
    add     x16, x12, x14               // w = l + q = 16q mod B
    lsr     x15, x14, #60               // hi = q >> 60
    cinc    x15, x15, lo                // hi + borrow
    cmp     x16, x14                    // C = [w >= q]
    sbc     x10, x15, xzr               // c' = hi + borrow - [w < q]
    subs    x17, x17, #1
    b.ne    Lby15_scalar

Lby15_groups:
    lsr     x2, x2, #2
    cbz     x2, Lby15_ret

.p2align 2
Lby15_loop:
    ldp     x3, x4, [x1]
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0]
    add     x16, x12, x14               // w = l + q
    lsr     x15, x14, #60
    cinc    x15, x15, lo
    cmp     x16, x14
    sbc     x10, x15, xzr

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #8]
    add     x16, x12, x14               // w = l + q
    lsr     x15, x14, #60
    cinc    x15, x15, lo
    cmp     x16, x14
    sbc     x10, x15, xzr

    ldp     x3, x4, [x1, #16]
    subs    x12, x3, x10
    mul     x14, x12, x8
    str     x14, [x0, #16]
    add     x16, x12, x14               // w = l + q
    lsr     x15, x14, #60
    cinc    x15, x15, lo
    cmp     x16, x14
    sbc     x10, x15, xzr

    subs    x12, x4, x10
    mul     x14, x12, x8
    str     x14, [x0, #24]
    add     x16, x12, x14               // w = l + q
    lsr     x15, x14, #60
    cinc    x15, x15, lo
    cmp     x16, x14
    sbc     x10, x15, xzr

    add     x1, x1, #32
    add     x0, x0, #32
    subs    x2, x2, #1
    b.ne    Lby15_loop

Lby15_ret:
    ret

#  Copyright (C) 2026 HJimmyK(Jericho Knox)
#
#  This file is part of LMMP.
#
#  LMMP is free software: you can redistribute it and/or modify it under
#  the terms of the GNU Lesser General Public License (LGPL) as published
#  by the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed WITHOUT ANY WARRANTY.
#
#  See <https://www.gnu.org/licenses/>.

# void lmmp_mul_basecase_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_srcptr numb, mp_size_t nb);

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mul_basecase_)
.p2align 2

ASM_GSYM(lmmp_mul_basecase_):
    stp     x29, x30, [sp, #-80]!
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    mov     x19, x0
    mov     x20, x1
    mov     x21, x2
    mov     x22, x3
    mov     x23, x4
    ldr     x24, [x22]
    mov     x0, x19
    mov     x1, x20
    mov     x2, x21
    mov     x3, x24
    bl      ASM_GSYM(lmmp_mul_1_)
    str     x0, [x19, x21, lsl #3]
    mov     x24, #1
    cmp     x24, x23
    b.hs    Lmb_done
.p2align 4

Lmb_outer:
    ldr     x3, [x22, x24, lsl #3]
    add     x0, x19, x24, lsl #3
    mov     x1, x20
    mov     x2, x21
    adds    x15, xzr, xzr
    tbz     x2, #0, Lmb_am_bx0
    ldr     x4, [x1], #8
    mul     x8, x4, x3
    umulh   x12, x4, x3
    ldr     x4, [x0]
    adds    x8, x4, x8
    cinc    x15, x12, cs
    str     x8, [x0], #8

Lmb_am_bx0:
    tbz     x2, #1, Lmb_am_2
    ldp     x4, x5, [x1], #16
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    adcs    x9, x9, x12
    ldp     x4, x5, [x0]
    adc     x15, x13, xzr
    adds    x8, x4, x8
    adcs    x9, x5, x9
    cinc    x15, x15, cs
    stp     x8, x9, [x0], #16

Lmb_am_2:
    lsr     x2, x2, #2
    cbz     x2, Lmb_am_le3
    ldp     x4, x5, [x1], #32
    ldp     x6, x7, [x1, #-16]
    b       Lmb_am_mid

Lmb_am_le3:
    mov     x0, x15
    b       Lmb_am_end
.p2align 4

Lmb_am_top:
    ldp     x4, x5, [x1], #32
    ldp     x6, x7, [x1, #-16]
    adds    x8, x16, x8
    adcs    x9, x17, x9
    stp     x8, x9, [x0], #32
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    stp     x10, x11, [x0, #-16]
    cinc    x15, x15, cs

Lmb_am_mid:
    sub     x2, x2, #1
    mul     x8, x4, x3
    umulh   x12, x4, x3
    mul     x9, x5, x3
    umulh   x13, x5, x3
    adds    x8, x8, x15
    mul     x10, x6, x3
    umulh   x14, x6, x3
    adcs    x9, x9, x12
    mul     x11, x7, x3
    umulh   x15, x7, x3
    adcs    x10, x10, x13
    ldp     x16, x17, [x0]
    adcs    x11, x11, x14
    ldp     x12, x13, [x0, #16]
    adc     x15, x15, xzr
    cbnz    x2, Lmb_am_top
    adds    x8, x16, x8
    adcs    x9, x17, x9
    adcs    x10, x12, x10
    adcs    x11, x13, x11
    stp     x8, x9, [x0]
    stp     x10, x11, [x0, #16]
    cinc    x0, x15, cs

Lmb_am_end:
    add     x5, x21, x24
    str     x0, [x19, x5, lsl #3]
    add     x24, x24, #1
    cmp     x24, x23
    b.lo    Lmb_outer

Lmb_done:
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

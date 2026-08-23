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

# mp_limb_t lmmp_mod_1_(mp_srcptr numa, mp_size_t na, mp_limb_t x);

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_mod_1_)
.p2align 2

ASM_GSYM(lmmp_mod_1_):
    stp     x29, x30, [sp, #-80]!
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    stp     x23, x24, [sp, #48]
    mov     x22, x2
    mov     x21, x1
    sub     x21, x21, #1
    mov     x20, x0
    add     x20, x20, x21, lsl #3
    tbnz    x22, #63, Lmod_normalised

Lmod_unnorm:
    clz     x23, x22
    lsl     x0, x22, x23
    bl      ASM_GSYM(lmmp_inv_1_)

Lmod_uentry:
    lsl     x22, x22, x23
    ldr     x7, [x20], #-8
    sub     x8, xzr, x23
    lsr     x11, x7, x8
    lsl     x1, x7, x23
    cbz     x21, Lmod_uend

Lmod_utop:
    ldr     x7, [x20], #-8
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    lsr     x9, x7, x8
    orr     x1, x1, x9
    adds    x10, x1, x10
    adc     x2, x2, x17
    msub    x11, x22, x2, x1
    lsl     x1, x7, x23
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc
    sbc     x2, x2, xzr
    cmp     x11, x22
    bcs     Lmod_ufx

Lmod_uok:
    sub     x21, x21, #1
    cbnz    x21, Lmod_utop

Lmod_uend:
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    adds    x10, x1, x10
    adc     x2, x2, x17
    msub    x11, x22, x2, x1
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc
    sbc     x2, x2, xzr
    subs    x14, x11, x22
    adc     x2, x2, xzr
    csel    x11, x14, x11, cs
    lsr     x0, x11, x23
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

Lmod_ufx:
    add     x2, x2, #1
    sub     x11, x11, x22
    b       Lmod_uok

Lmod_normalised:
    mov     x0, x22
    bl      ASM_GSYM(lmmp_inv_1_)

Lmod_nentry:
    ldr     x7, [x20], #-8
    subs    x14, x7, x22
    csel    x11, x14, x7, cs
    b       Lmod_nok

Lmod_ntop:
    ldr     x1, [x20], #-8
    add     x2, x11, #1
    mul     x10, x11, x0
    umulh   x17, x11, x0
    adds    x10, x1, x10
    adc     x2, x2, x17
    msub    x11, x22, x2, x1
    cmp     x10, x11
    add     x14, x11, x22
    csel    x11, x14, x11, cc
    sbc     x2, x2, xzr
    cmp     x11, x22
    bcs     Lmod_nfx

Lmod_nok:
    sub     x21, x21, #1
    tbz     x21, #63, Lmod_ntop
    mov     x0, x11
    ldp     x19, x20, [sp, #16]
    ldp     x21, x22, [sp, #32]
    ldp     x23, x24, [sp, #48]
    ldp     x29, x30, [sp], #80
    ret

Lmod_nfx:
    add     x2, x2, #1
    sub     x11, x11, x22
    b       Lmod_nok

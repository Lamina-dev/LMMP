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

# mp_limb_t lmmp_shr_c_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr, mp_limb_t c);
# mp_limb_t lmmp_shr_(mp_ptr dst, mp_srcptr numa, mp_size_t na, mp_size_t shr);

#include "lmmp_asm.h"

.text

.globl ASM_GSYM(lmmp_shr_c_)
.p2align 2

ASM_GSYM(lmmp_shr_c_):
    mov     x15, x4
    b       Lshr_ent

.globl ASM_GSYM(lmmp_shr_)
.p2align 2

ASM_GSYM(lmmp_shr_):
    mov     x15, xzr

Lshr_ent:
    cbz     x3, Lshr_copy
    mov     x16, x0
    sub     x8, xzr, x3
    lsr     x17, x2, #2
    tbz     x2, #0, Lshr_bx0

Lshr_bx1:
    ldr     x5, [x1]
    tbnz    x2, #1, Lshr_b11

Lshr_b01:
    lsl     x0, x5, x8
    lsr     x2, x5, x3
    cbnz    x17, Lshr_gt1
    orr     x2, x2, x15
    str     x2, [x16]
    ret

Lshr_gt1:
    ldp     x4, x5, [x1, #8]
    sub     x1, x1, #8
    sub     x16, x16, #32
    b       Lshr_lo2

Lshr_b11:
    lsl     x0, x5, x8
    lsr     x2, x5, x3
    ldp     x6, x7, [x1, #8]!
    sub     x16, x16, #16
    b       Lshr_lo3

Lshr_bx0:
    ldp     x4, x5, [x1]
    tbz     x2, #1, Lshr_b00

Lshr_b10:
    lsl     x0, x4, x8
    lsr     x13, x4, x3
    lsl     x10, x5, x8
    lsr     x2, x5, x3
    cbnz    x17, Lshr_gt2
    orr     x10, x10, x13
    orr     x2, x2, x15
    stp     x10, x2, [x16]
    ret

Lshr_gt2:
    ldp     x4, x5, [x1, #16]
    orr     x10, x10, x13
    str     x10, [x16], #-24
    b       Lshr_lo2

Lshr_b00:
    lsl     x0, x4, x8
    lsr     x13, x4, x3
    lsl     x10, x5, x8
    lsr     x2, x5, x3
    ldp     x6, x7, [x1, #16]!
    orr     x10, x10, x13
    str     x10, [x16], #-8
    b       Lshr_lo0
.p2align 4

Lshr_top:
    ldp     x4, x5, [x1, #16]
    orr     x10, x10, x13
    orr     x11, x12, x2
    stp     x11, x10, [x16, #16]
    lsr     x2, x7, x3

Lshr_lo2:
    lsl     x10, x5, x8
    lsl     x12, x4, x8
    lsr     x13, x4, x3
    ldp     x6, x7, [x1, #32]!
    orr     x10, x10, x13
    orr     x11, x12, x2
    stp     x11, x10, [x16, #32]!
    lsr     x2, x5, x3

Lshr_lo0:
    sub     x17, x17, #1

Lshr_lo3:
    lsl     x10, x7, x8
    lsl     x12, x6, x8
    lsr     x13, x6, x3
    cbnz    x17, Lshr_top

Lshr_end:
    orr     x10, x10, x13
    orr     x11, x12, x2
    lsr     x2, x7, x3
    stp     x11, x10, [x16, #16]
    orr     x2, x2, x15
    str     x2, [x16, #32]
    ret

Lshr_copy:
    cmp     x0, x1
    b.eq    Lshr_copy_done
    mov     x6, x2
    lsr     x7, x2, #2
    cbz     x7, Lshr_copy_tail
.p2align 4

Lshr_copy_loop:
    ldp     x8, x9, [x1], #16
    stp     x8, x9, [x0], #16
    ldp     x8, x9, [x1], #16
    stp     x8, x9, [x0], #16
    sub     x7, x7, #1
    cbnz    x7, Lshr_copy_loop

Lshr_copy_tail:
    ands    x6, x6, #3
    b.eq    Lshr_copy_done

Lshr_copy_tail_loop:
    ldr     x8, [x1], #8
    str     x8, [x0], #8
    sub     x6, x6, #1
    cbnz    x6, Lshr_copy_tail_loop

Lshr_copy_done:
    mov     x0, xzr
    ret
